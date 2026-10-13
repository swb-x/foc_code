#include "app_adc.h"
#include "cmsis_os.h"
#include "stm32g4xx_ll_adc.h"
#include "stm32g4xx_ll_dma.h"

volatile uint16_t temp_raw = 0;      /* DMA 搬运目标，半字对齐 */
volatile uint8_t  temp_done = 0;     /* 转换完成标志 */

/* DMA1 通道2 传输完成回调（在 it.c 的 DMA1_Channel2_IRQHandler 里被调用） */
void App_ADC2_ConvCplt(void)
{
    temp_done = 1;
}

/* ADC2 一次性初始化：校准 + 使能 + DMA 通道配置 */
static void Temp_ADC2_Init(void)
{
    /* 1. 校准（要求 ADC 关闭状态，MX_ADC2_Init 之后正是关闭状态） */
    LL_ADC_StartCalibration(ADC2, LL_ADC_SINGLE_ENDED);
    while (LL_ADC_IsCalibrationOnGoing(ADC2) != 0U){}

    /* 2. 使能 ADC2，等待就绪 */
    LL_ADC_Enable(ADC2);
    while (LL_ADC_IsActiveFlag_ADRDY(ADC2) == 0U){}

    /* 3. 配置 DMA1 通道2：ADC2_DR → temp_raw，长度 1，开传输完成中断 */
    LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_2,
                           LL_ADC_DMA_GetRegAddr(ADC2, LL_ADC_DMA_REG_REGULAR_DATA),
                           (uint32_t)&temp_raw,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, 1);
    LL_DMA_ClearFlag_TC2(DMA1);
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_2);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
}

void ADC1_Current_Start(uint16_t *buf, uint32_t len)
{
    /* 1. 校准（ADC 关闭状态下） */
    LL_ADC_StartCalibration(ADC1, LL_ADC_SINGLE_ENDED);
    while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0U) {}

    /* 2. 使能，等就绪 */
    LL_ADC_Enable(ADC1);
    while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0U) {}

    /* 3. DMA1 通道1：ADC1_DR → buf，循环模式 */
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, len);
    LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_1,
                           LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA),
                           (uint32_t)buf,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
    

    /* 4. 使能 ADC 的 DMA 请求（UNLIMITED 模式下一次即可） */
    LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);

    /* 5. 启动：之后由 TIM1 的 OC4REF 每 50?s 自动触发一帧 */
    LL_ADC_REG_StartConversion(ADC1);
}


void StartTempDetectTask(void *argument)
{
    printf("TempDetectTask started\n");
    Temp_ADC2_Init();
    printf("adc2 calibration & dma init done\n");

    for (;;)
    {
        temp_done = 0;
        /* DMA 单次模式下，传输完成后硬件会清零 DMAEN，每次转换前需重开 */
        LL_ADC_REG_SetDMATransfer(ADC2, LL_ADC_REG_DMA_TRANSFER_LIMITED);
        /* 软件触发一次转换，结果由 DMA 搬到 temp_raw，完成后进中断置标志 */
        LL_ADC_REG_StartConversion(ADC2);

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
            /* R0 和 B 值以实际 NTC 规格书为准 */
            float tempC = 1.0f / (logf(rntc / 50000.0f) / 3950.0f + 1.0f / 298.15f) - 273.15f;
            printf("raw=%u  V=%.3fV  Rntc=%.0f  T=%.2fC\r\n", temp_raw, vtemp, rntc, tempC);
        }
        else if (!temp_done)
        {
            printf("ADC2 timeout!\r\n");
        }

        osDelay(1000);
    }
}
