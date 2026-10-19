#include "mcpwm_foc.h"
#include "app_adc.h"   /* ADC1_Current_Start */
#include "foc_math.h"
#include "cmsis_os.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_adc.h"
#include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_tim.h"

/* 故障状态：nFAULT 拉低后置位，必须显式调用 Motor_ClearFault() 才允许重新输出 */
static volatile bool m_fault_latched = false;
volatile uint32_t g_drv_fault_count = 0;

/* ADC1 DMA 帧缓冲与快照：[0]=U(SOA) [1]=V(SOB) [2]=W(SOC) [3]=Vbus */
static uint16_t adc_buf[4];
volatile uint16_t g_adc_raw[4];

#define FOC_DT   (1.0f / 20000.0f)   /* 电流环周期 = PWM 周期 50us */

static void stop_pwm_hw(void);
static void start_pwm_hw(void);

/*==========================================================================
 * PWM 硬件层（示波器已验证）
 *==========================================================================*/

/* 50% 占空比输出测试：三相同相位、同占空比 */
void Motor_PWM_Test50(void)
{
    TIMER_UPDATE_DUTY_MT(2125, 2125, 2125);   /* ARR=4249 → 50% */
    start_pwm_hw();
    LL_TIM_EnableCounter(TIM1);
    LL_GPIO_SetOutputPin(ENABLE_GPIO_Port, ENABLE_Pin);
    osDelay(2);
    LL_TIM_EnableAllOutputs(TIM1);
    printf("PWM 50%% output ON\r\n");
}

/* PWM输出 */
void Motor_PWM_Start(void)
{
    Motor_ClearFault();
    TIMER_UPDATE_DUTY_MT(2125, 2125, 2125);
    start_pwm_hw();
    LL_TIM_EnableCounter(TIM1);
    LL_GPIO_SetOutputPin(ENABLE_GPIO_Port, ENABLE_Pin);
    osDelay(2);
    LL_TIM_EnableAllOutputs(TIM1);
}

/* 停止输出：先关 MOE，再拉低 ENABLE */
void Motor_PWM_Stop(void)
{
    LL_TIM_DisableAllOutputs(TIM1);
    stop_pwm_hw();
    LL_GPIO_ResetOutputPin(ENABLE_GPIO_Port, ENABLE_Pin);
    LL_TIM_DisableCounter(TIM1);
}

/*==========================================================================
 * nFAULT 紧急关断
 * 由 EXTI9_5_IRQHandler（PA5 下降沿）直接调用，保持 ISR 安全：
 * 不调用任何 RTOS API、不阻塞、不打印。
 *==========================================================================*/
void Motor_PWM_FaultStop(void)
{
    /* 1. 六路 OC 全部强制无效电平：CCxE/CCxNE 保持使能，
     *    栅极被"主动拉低"而不是高阻浮空（高有效时无效电平=低）。 */
    stop_pwm_hw();

    /* 2. 拉低 ENABLE：DRV8353 进入休眠，电荷泵停摆，栅极驱动彻底关闭 */
    LL_GPIO_ResetOutputPin(ENABLE_GPIO_Port, ENABLE_Pin);

    /* 3. 清 MOE 并停计数器，防止 ENABLE 被意外拉高后又瞬间输出 */
    LL_TIM_DisableAllOutputs(TIM1);
    LL_TIM_DisableCounter(TIM1);

    m_fault_latched = true;
}

bool Motor_IsFaultLatched(void)
{
    return m_fault_latched;
}

void Motor_ClearFault(void)
{
    m_fault_latched = false;
}

/*==========================================================================
 * FOC 初始化：参数装载 + ADC1 校准 + DMA + 触发启动
 * 任务上下文调用，整个系统只调一次。
 *==========================================================================*/
void FOC_Init(void)
{
    MC_LoadDefaults();

    ADC1_Current_Start(adc_buf, 4);
}

/*==========================================================================
 * FOC 电流环：20kHz，DMA1_Ch1 TC 中断里调用
 * IDLE 模式下只做采样遥测，不驱动 PWM。
 *==========================================================================*/
void FOC_ControlLoop(void)
{
    mc_cfg_t   *c = &g_mc_cfg;
    mc_state_t *s = &g_mc_state;

    /* 1. 采样换算 */
    for (int i = 0; i < 4; i++) {
        g_adc_raw[i] = adc_buf[i];
    }
    float iu = ((float)adc_buf[0] - c->i_offset[0]) * c->i_scale;
    float iv = ((float)adc_buf[1] - c->i_offset[1]) * c->i_scale;
    float iw = ((float)adc_buf[2] - c->i_offset[2]) * c->i_scale;

    /* 三相之和不为零的部分视为共模误差，均摊扣除 */
    float icom = (iu + iv + iw) * (1.0f / 3.0f);
    s->iu = iu - icom;
    s->iv = iv - icom;
    s->iw = iw - icom;
    s->vbus = (float)adc_buf[3] * c->vbus_scale;

    /* 故障锁存：输出已在 EXTI 里强制关闭，这里只清控制量，等上层显式恢复 */
    if (m_fault_latched) {
        s->fault   = true;
        s->mode    = FOC_MODE_IDLE;
        s->iq_set  = 0.0f;
        s->id_set  = 0.0f;
        s->vd_int  = 0.0f;
        s->vq_int  = 0.0f;
        return;
    }
    s->fault = false;

    /* 2. Clarke */
    foc_clarke(s->iu, s->iv, &s->i_alpha, &s->i_beta);

    /* 3. 观测器 + PLL 始终并行运行（用上一拍的指令电压重构） */
    foc_observer_update(s->v_alpha, s->v_beta, s->i_alpha, s->i_beta,
                        c->R, c->L, c->lambda, FOC_DT,
                        &s->obs, &s->obs_phase);
    foc_pll_run(s->obs_phase, c->pll_kp, c->pll_ki, FOC_DT,
                &s->pll_phase, &s->pll_speed);

    if (s->mode == FOC_MODE_IDLE) {
        return;
    }

    /* 4. 相位来源：开环拖转 / 观测器闭环 */
    if (!s->use_observer) {
        s->ol_phase += s->ol_speed * FOC_DT;
        s->ol_phase  = foc_norm_angle(s->ol_phase);

        /* 只有观测器磁链幅值健康（说明已收敛）且转速够高，才允许切闭环 */
        float erpm    = s->pll_speed * 60.0f / FOC_2PI;
        float obs_mag = NORM2_f(s->obs.x1, s->obs.x2);
        if (fabsf(erpm) > c->obs_switch_erpm && obs_mag > (0.5f * c->lambda)) {
            s->use_observer = true;
        }
    }
    s->phase = s->use_observer ? s->obs_phase : s->ol_phase;

    /* 5. Park */
    foc_park(s->i_alpha, s->i_beta, s->phase, &s->id, &s->iq);

    /* 6. 电流环 PI + dq 解耦 */
    float err_d = s->id_set - s->id;
    float err_q = s->iq_set - s->iq;

    float vmax = s->vbus * ONE_BY_SQRT3;        /* SVM 线性区电压圆 */
    s->vd_int = foc_clamp(s->vd_int + err_d * c->cur_ki * FOC_DT, -vmax, vmax);
    s->vq_int = foc_clamp(s->vq_int + err_q * c->cur_ki * FOC_DT, -vmax, vmax);

    float w  = s->pll_speed;                     /* 电气角速度 */
    float vd = err_d * c->cur_kp + s->vd_int - w * c->L * s->iq;
    float vq = err_q * c->cur_kp + s->vq_int + w * (c->L * s->id + c->lambda);

    /* 电压圆限幅（等比缩放，保持方向） */
    float vmag = NORM2_f(vd, vq);
    if (vmag > vmax) {
        vd *= vmax / vmag;
        vq *= vmax / vmag;
    }
    s->vd = vd;
    s->vq = vq;

    /* 7. 反 Park + SVM + 写占空比 */
    foc_inv_park(vd, vq, s->phase, &s->v_alpha, &s->v_beta);

    /* 母线未上电（或采样异常）时输出中点，避免 SVM 除零 */
    if (s->vbus < 3.0f) {
        uint32_t mid = TIM1->ARR / 2;
        TIMER_UPDATE_DUTY_MT(mid, mid, mid);
        return;
    }

    uint32_t du, dv, dw;
    foc_svm(s->v_alpha, s->v_beta, s->vbus, c->duty_max, TIM1->ARR, &du, &dv, &dw);
    TIMER_UPDATE_DUTY_MT(du, dv, dw);
}

/*==========================================================================
 * FOC 速度环：1kHz，任务里调用
 *==========================================================================*/
void FOC_SpeedLoop(void)
{
    mc_cfg_t   *c = &g_mc_cfg;
    mc_state_t *s = &g_mc_state;

    if (s->mode != FOC_MODE_SPEED) {
        return;
    }

    /* 电气 rad/s → 机械 rpm */
    float spd_rpm = s->pll_speed * 60.0f / FOC_2PI / (float)c->pole_pairs;

    float err = s->spd_set_rpm - spd_rpm;
    s->spd_int = foc_clamp(s->spd_int + err * c->spd_ki * 0.001f, -c->i_max, c->i_max);
    s->iq_set  = foc_clamp(err * c->spd_kp + s->spd_int, -c->i_max, c->i_max);
    s->id_set  = 0.0f;
}

/*==========================================================================
 * 运行控制
 *==========================================================================*/

/* 开环拖转启动：iq 给定 + 开环相位以 erpm（电气 rpm）旋转，观测器并行跟踪，
 * 转速超过 obs_switch_erpm 后自动切到观测器相位。 */
void FOC_StartOpenLoop(float iq, float erpm)
{
    mc_state_t *s = &g_mc_state;

    if (m_fault_latched) {
        return;
    }

    s->id_set       = 0.0f;
    s->iq_set       = iq;
    s->ol_speed     = erpm / 60.0f * FOC_2PI;   /* 电气 rpm → rad/s */
    s->ol_phase     = 0.0f;
    s->use_observer = false;
    s->vd_int       = 0.0f;
    s->vq_int       = 0.0f;
    s->mode         = FOC_MODE_OPENLOOP;

    /* 先给 50% 中点，再开输出（ISR 会立即接管占空比） */
    TIMER_UPDATE_DUTY_MT(TIM1->ARR / 2, TIM1->ARR / 2, TIM1->ARR / 2);
    start_pwm_hw();
    LL_TIM_EnableCounter(TIM1);
    LL_GPIO_SetOutputPin(ENABLE_GPIO_Port, ENABLE_Pin);
    osDelay(2);
    LL_TIM_EnableAllOutputs(TIM1);
}

void FOC_SetSpeed(float rpm)
{
    mc_state_t *s = &g_mc_state;

    /* 速度闭环依赖观测器收敛，必须先由 FOC_StartOpenLoop() 把转子拖起来 */
    if (s->mode == FOC_MODE_IDLE || m_fault_latched) {
        return;
    }

    s->spd_set_rpm = rpm;
    s->spd_int     = 0.0f;
    s->mode        = FOC_MODE_SPEED;
}

void FOC_Stop(void)
{
    g_mc_state.mode = FOC_MODE_IDLE;
    Motor_PWM_Stop();
}

/*==========================================================================
 * 内部：PWM 输出级开关
 *==========================================================================*/
static void stop_pwm_hw(void){
    //======== TIM1 CH1 =========
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_FORCED_INACTIVE);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);       // CC1E = 1 主通道使能
    LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH1N);     // CC1NE = 0 互补关闭

    //======== TIM1 CH2 =========
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_FORCED_INACTIVE);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
    LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2N);

    //======== TIM1 CH3 =========
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_FORCED_INACTIVE);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);
    LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH3N);

    // COM事件同步刷新三路OC配置，等价 TIM1->EGR |= TIM_EGR_COMG;
    LL_TIM_GenerateEvent_COM(TIM1);
}

static void start_pwm_hw(void){
   //======== TIM1 CH1 =========
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);       // CC1E = 1 主通道使能
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1N);      // CC1NE = 1 互补使能

    //======== TIM1 CH2 =========
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2N);

    //======== TIM1 CH3 =========
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3N);

    LL_TIM_GenerateEvent_COM(TIM1);
}
