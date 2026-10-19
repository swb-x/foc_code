#include "mc_config.h"
#include <stdio.h>           /* printf */
#include "cmsis_os.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_tim.h"

mc_cfg_t   g_mc_cfg;
mc_state_t g_mc_state;

/* ISR 更新的最新 ADC 原始值（定义在 mcpwm_foc.c）：[U, V, W, Vbus] */
extern volatile uint16_t g_adc_raw[4];

/**
 * @brief 装载默认参数。
 *        【注意】R / L / lambda / pole_pairs 是占位值，必须按实际电机修改，
 *        改完电机参数后观测器增益和 PI 会在此函数里自动重算。
 */
void MC_LoadDefaults(void)
{
    mc_cfg_t *c = &g_mc_cfg;

    /* ---- 电机参数（TODO：按实际电机实测填写） ---- */
    c->R          = 0.05f;     /* 相电阻 Ω */
    c->L          = 20e-6f;    /* 相电感 H */
    c->lambda     = 5e-3f;     /* 磁链 Wb */
    c->pole_pairs = 7;

    /* ---- 观测器 / PLL ---- */
    c->pll_kp          = 2000.0f;
    c->pll_ki          = 20000.0f;
    c->obs_switch_erpm = 500.0f;   /* 电气 rpm 超过此值切观测器 */

    /* ---- 电流环 PI：bw = 1000 rad/s ---- */
    const float bw = 1000.0f;
    c->cur_kp = c->L * bw;
    c->cur_ki = c->R * bw;

    /* ---- 速度环 PI（经验初值，从小往大调） ---- */
    c->spd_kp = 0.005f;
    c->spd_ki = 0.05f;

    /* ---- 限幅 ---- */
    c->i_max    = 5.0f;     /* A，按电机和散热定 */
    c->duty_max = 0.95f;    /* 低侧采样窗口限制 */

    /* ---- 采样标定（GAIN=10 V/V，Rshunt=5mΩ，分压 10K/1K） ---- */
    c->i_scale    = 3.3f / 4095.0f / (10.0f * 0.005f);   /* ≈0.01611 A/LSB */
    c->vbus_scale = 3.3f / 4095.0f * 11.0f;              /* ≈0.00886 V/LSB */

    /* 失调默认取理论中点，校准后覆盖 */
    c->i_offset[0] = 2048.0f;
    c->i_offset[1] = 2048.0f;
    c->i_offset[2] = 2048.0f;

    /* 状态清零 */
    g_mc_state.mode         = FOC_MODE_IDLE;
    g_mc_state.fault        = false;
    g_mc_state.use_observer = false;
    g_mc_state.id_set       = 0.0f;
    g_mc_state.iq_set       = 0.0f;
    g_mc_state.spd_set_rpm  = 0.0f;
}

/**
 * @brief DRV8353 电流失调校准（CAL 脚流程）。
 *        前提：FOC_Init() 已执行（ADC1+DMA 已启动）。
 *        期间 PWM 输出保持关闭（MOE=0），电机零电流。
 *        调用上下文：任务。
 */
void MC_CalibrateOffsets(void)
{
    /* 唤醒 DRV8353（MOE 仍为 0，功率级无输出） */
    LL_GPIO_SetOutputPin(ENABLE_GPIO_Port, ENABLE_Pin);
    osDelay(2);

    /* ADC 触发来自 TIM1 TRGO，计数器必须先转起来 */
    LL_TIM_EnableCounter(TIM1);
    osDelay(2);

    /* CAL 拉高：放大器输入内部短接，SOx 输出 = 纯失调电压 */
    LL_GPIO_SetOutputPin(CAL_GPIO_Port, CAL_Pin);
    osDelay(5);

    float sum[3] = {0.0f, 0.0f, 0.0f};
    const int N = 200;
    for (int i = 0; i < N; i++) {
        sum[0] += (float)g_adc_raw[0];
        sum[1] += (float)g_adc_raw[1];
        sum[2] += (float)g_adc_raw[2];
        osDelay(1);
    }

    LL_GPIO_ResetOutputPin(CAL_GPIO_Port, CAL_Pin);

    for (int ch = 0; ch < 3; ch++) {
        g_mc_cfg.i_offset[ch] = sum[ch] / (float)N;
    }

    printf("offset cal: %.1f %.1f %.1f\r\n",
           g_mc_cfg.i_offset[0], g_mc_cfg.i_offset[1], g_mc_cfg.i_offset[2]);
}
