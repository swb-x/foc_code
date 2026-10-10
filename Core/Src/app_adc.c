#include "app_adc.h"


volatile uint16_t temp_raw = 0;      /* DMA 搬运目标，半字对齐 */
volatile uint8_t  temp_done = 0;     /* 转换完成标志 */

/* ADC 转换完成回调（DMA 传输完成中断里被调用） */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC2)
    {
        temp_done = 1;
    }
}



void StartTempDetectTask(void *argument)
{
    printf("TempDetectTask started\n");
    /* 上电先校准 ADC2，只做一次 */
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
    printf("adc_calibration done\n");

    for (;;)
    {
        temp_done = 0;
        /* 启动一次转换，结果由 DMA 搬到 temp_raw，完成后进回调置标志 */
        HAL_ADC_Start_DMA(&hadc2, (uint32_t *)&temp_raw, 1);

        /* 等转换完成（12.5+12.5 周期 @42.5MHz ≈ 0.6?s，等 1ms 绰绰有余） */
        uint32_t timeout = 10;
        while (temp_done == 0 && timeout-- > 0)
        {
            osDelay(1);
        }

        if (temp_done && temp_raw > 0)   /* temp_raw>0 防除零 */
        {
            float vtemp = (float)temp_raw * 3.3f / 4095.0f;
            /* 分压公式反推 NTC 阻值：V = 3V3A × 3.3K/(Rntc+3.3K) */
            float rntc  = 3300.0f * (4095.0f / (float)temp_raw - 1.0f);
            float tempC = 1.0f / (logf(rntc / 10000.0f) / 3380.0f + 1.0f / 298.15f) - 273.15f;
            printf("raw=%u  V=%.3fV  Rntc=%.0f  T=%.2fC\r\n", temp_raw, vtemp, rntc, tempC);
        }
        else if (!temp_done)
        {
            printf("ADC2 timeout!\r\n");
        }

        osDelay(1000);
    }
}



