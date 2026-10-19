#ifndef __APP_ADC_H__
#define __APP_ADC_H__

#include "main.h"
#include "adc.h"

void StartTempDetectTask(void *argument);
void App_ADC2_ConvCplt(void);

/* ADC1 电流采样启动：校准 + 使能 + DMA1_Ch1 循环搬运 + TIM1 触发
   buf 需为 4 个半字（U, V, W, Vbus），任务上下文调用，只调一次 */
void ADC1_Current_Start(uint16_t *buf, uint32_t len);

#endif