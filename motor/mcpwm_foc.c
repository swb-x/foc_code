#include "mcpwm_foc.h"

/* 50% 占空比输出测试：三相同相位、同占空比 */
void Motor_PWM_Test50(void)
{
    /* 1. 占空比 50%：ARR=4249 → CCR=2125 */
    TIMER_UPDATE_DUTY_MT(2125, 2125, 2125);

    /* 2. OCxM 切回 PWM1 + 打开互补输出（你已有的函数） */
    start_pwm_hw();

    /* 3. 先启动计数器（MOE 还没开，引脚仍是安全的无效电平） */
    __HAL_TIM_ENABLE(&htim1);

    /* 4. 使能 DRV8353，等电荷泵/DVDD 就绪 */
    // HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_SET);
    // osDelay(2);

    /* 5. 最后开 MOE，六路 PWM 同时上线 */
    __HAL_TIM_MOE_ENABLE(&htim1);

    printf("PWM 50%% output ON\r\n");
}

/* 停止输出：先关 MOE，再拉低 ENABLE */
void Motor_PWM_Stop(void)
{
    __HAL_TIM_MOE_DISABLE(&htim1);
    stop_pwm_hw();
    HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);
    __HAL_TIM_DISABLE(&htim1);
}



static void stop_pwm_hw(void){
    // TIM1 通道1
    TIM1->CCMR1 &= ~(TIM_CCMR1_OC1M);
    TIM1->CCMR1 |=  TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1; // OC1M=011：强制无效 Inactive
    TIM1->CCER  |=  TIM_CCER_CC1E;                       // CC1E=1 主通道使能
    TIM1->CCER  &= ~TIM_CCER_CC1NE;                      // CC1NE=0 互补通道关闭

    // TIM1 通道2
    TIM1->CCMR1 &= ~(TIM_CCMR1_OC2M);
    TIM1->CCMR1 |=  TIM_CCMR1_OC2M_0 | TIM_CCMR1_OC2M_1; // OC2M=011
    TIM1->CCER  |=  TIM_CCER_CC2E;
    TIM1->CCER  &= ~TIM_CCER_CC2NE;

    // TIM1 通道3
    TIM1->CCMR2 &= ~(TIM_CCMR2_OC3M);
    TIM1->CCMR2 |=  TIM_CCMR2_OC3M_0 | TIM_CCMR2_OC3M_1; // OC3M=011
    TIM1->CCER  |=  TIM_CCER_CC3E;
    TIM1->CCER  &= ~TIM_CCER_CC3NE;

    TIM1->EGR |= TIM_EGR_COMG; // 触发COM事件，同步刷新三路OC配置
}

static void start_pwm_hw(void){
    // CH1 CH2 CH3 切回PWM模式1 OCxM=110
    TIM1->CCMR1 &= ~(TIM_CCMR1_OC1M);
    TIM1->CCMR1 |=  TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;

    TIM1->CCMR1 &= ~(TIM_CCMR1_OC2M);
    TIM1->CCMR1 |=  TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;

    TIM1->CCMR2 &= ~(TIM_CCMR2_OC3M);
    TIM1->CCMR2 |=  TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2;

    TIM1->CCER |= TIM_CCER_CC1NE | TIM_CCER_CC2NE | TIM_CCER_CC3NE; // 打开互补输出

    TIM1->EGR |= TIM_EGR_COMG; // COM同步生效
}
