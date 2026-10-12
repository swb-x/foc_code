#include "mcpwm_foc.h"

/* 50% 占空比输出测试：三相同相位、同占空比 */
void Motor_PWM_Test50(void)
{
    /* 1. 占空比 50%：ARR=4249 → CCR=2125 */
    TIMER_UPDATE_DUTY_MT(2125, 2125, 2125);

    /* 2. OCxM 切回 PWM1 + 打开互补输出（你已有的函数） */
    start_pwm_hw();

    /* 3. 先启动计数器（MOE 还没开，引脚仍是安全的无效电平） */
    LL_TIM_EnableCounter(TIM1);

    /* 4. 使能 DRV8353，等电荷泵/DVDD 就绪 */
    // HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_SET);
    // osDelay(2);

    /* 5. 最后开 MOE，六路 PWM 同时上线 */
    LL_TIM_EnableMainOutput(TIM1);

    printf("PWM 50%% output ON\r\n");
}

/* PWM输出 */
void Motor_PWM_Start(void)
{
    TIMER_UPDATE_DUTY_MT(2125, 2125, 2125);
    start_pwm_hw();
    LL_TIM_EnableCounter(TIM1);
    HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_SET);
    osDelay(2);
    LL_TIM_EnableMainOutput(TIM1);
}
/* 停止输出：先关 MOE，再拉低 ENABLE */
void Motor_PWM_Stop(void)
{
    LL_TIM_DisableMainOutput(TIM1);
    stop_pwm_hw();
    HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);
    LL_TIM_DisableCounter(TIM1);
}



static void stop_pwm_hw(void){
    //======== TIM1 CH1 =========
    LL_TIM_SetOCMode(TIM1, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_FORCED_INACTIVE);
    LL_TIM_EnableCCxOutput(TIM1, LL_TIM_CHANNEL_CH1);      // CC1E = 1 主通道使能
    LL_TIM_DisableCCxNOutput(TIM1, LL_TIM_CHANNEL_CH1);    // CC1NE = 0 互补关闭

    //======== TIM1 CH2 =========
    LL_TIM_SetOCMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_FORCED_INACTIVE);
    LL_TIM_EnableCCxOutput(TIM1, LL_TIM_CHANNEL_CH2);
    LL_TIM_DisableCCxNOutput(TIM1, LL_TIM_CHANNEL_CH2);

    //======== TIM1 CH3 =========
    LL_TIM_SetOCMode(TIM1, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_FORCED_INACTIVE);
    LL_TIM_EnableCCxOutput(TIM1, LL_TIM_CHANNEL_CH3);
    LL_TIM_DisableCCxNOutput(TIM1, LL_TIM_CHANNEL_CH3);

    // COM事件同步刷新三路OC配置，等价 TIM1->EGR |= TIM_EGR_COMG;
    LL_TIM_GenerateEvent_COM(TIM1);
}

static void start_pwm_hw(void){
   //======== TIM1 CH1 =========
    LL_TIM_SetOCMode(TIM1, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    LL_TIM_EnableCCxOutput(TIM1, LL_TIM_CHANNEL_CH1);      // CC1E = 1 主通道使能
    LL_TIM_EnableCCxNOutput(TIM1, LL_TIM_CHANNEL_CH1);    // CC1NE = 0 互补关闭

    //======== TIM1 CH2 =========
    LL_TIM_SetOCMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
    LL_TIM_EnableCCxOutput(TIM1, LL_TIM_CHANNEL_CH2);
    LL_TIM_EnableCCxNOutput(TIM1, LL_TIM_CHANNEL_CH2);

    //======== TIM1 CH3 =========
    LL_TIM_SetOCMode(TIM1, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
    LL_TIM_EnableCCxOutput(TIM1, LL_TIM_CHANNEL_CH3);
    LL_TIM_EnableCCxNOutput(TIM1, LL_TIM_CHANNEL_CH3);

    LL_TIM_GenerateEvent_COM(TIM1);
}
