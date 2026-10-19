#ifndef __MCPWM_FOC_H__
#define __MCPWM_FOC_H__

#include "main.h"
#include "tim.h"
#include "mc_config.h"

/* 三路占空比同步写入：CCR1=U, CCR2=V, CCR3=W（与 GHx→U/V/W 接线一致） */
#define TIMER_UPDATE_DUTY_MT(duty1, duty2, duty3) \
		TIM1->CR1 |= TIM_CR1_UDIS; \
		TIM1->CCR1 = duty1; \
		TIM1->CCR2 = duty2; \
		TIM1->CCR3 = duty3; \
		TIM1->CR1 &= ~TIM_CR1_UDIS;

void Motor_PWM_Test50(void);
void Motor_PWM_Start(void);
void Motor_PWM_Stop(void);

/* nFAULT 触发时的紧急关断：中断上下文可直接调用（内部不使用任何 RTOS/阻塞接口） */
void Motor_PWM_FaultStop(void);
bool Motor_IsFaultLatched(void);
void Motor_ClearFault(void);
extern volatile uint32_t g_drv_fault_count;

/* 最新一帧 ADC1 原始值：[0]=U(SOA) [1]=V(SOB) [2]=W(SOC) [3]=Vbus */
extern volatile uint16_t g_adc_raw[4];

/*--------------------------------------------------------------------------
 * FOC 主接口
 * 调用顺序：FOC_Init() → MC_CalibrateOffsets() → FOC_StartOpenLoop() → FOC_SetSpeed()
 *-------------------------------------------------------------------------*/
void FOC_Init(void);            /* 参数装载 + ADC1 校准 + DMA + 触发启动（任务上下文，一次） */
void FOC_ControlLoop(void);     /* 20kHz 电流环，在 DMA1_Ch1 TC 中断里调用 */
void FOC_SpeedLoop(void);       /* 1kHz 速度环，在 1ms 任务里调用 */
void FOC_StartOpenLoop(float iq, float erpm);   /* 开环拖转启动（电流闭环 + 开环相位） */
void FOC_SetSpeed(float rpm);   /* 切速度闭环并设定机械转速 */
void FOC_Stop(void);            /* 回 IDLE 并关闭输出 */

#endif