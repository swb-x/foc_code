#ifndef __MCPWM_FOC_H__
#define __MCPWM_FOC_H__

#include "main.h"
#include "tim.h"

#define TIMER_UPDATE_DUTY_MT(duty1, duty2, duty3) \
		TIM1->CR1 |= TIM_CR1_UDIS; \
		TIM1->CCR1 = duty1; \
		TIM1->CCR2 = duty3; \
		TIM1->CCR3 = duty2; \
		TIM1->CR1 &= ~TIM_CR1_UDIS;

void Motor_PWM_Test50(void);
void Motor_PWM_Start(void);
void Motor_PWM_Stop(void);
void mcpwm_foc_dc_cal(void);


#endif