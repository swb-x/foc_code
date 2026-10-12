#include "mc_config.h"


void mc_config_init(void)
{
    // 初始化电机参数
    memset(&mc_config, 0, sizeof(mc_configuration));
    
    // 初始化DRV8323
    drv8323_init();
}

void drv8323_init(void)
{
    /* 失调电压校准 */
    HAL_GPIO_WritePin(CAL_GPIO_Port, CAL_Pin, GPIO_PIN_SET);
    osDelay(1); // 延时1ms 等待校准完成

    const float samples = 1000.0;
	float current_sum[3] = {0.0, 0.0, 0.0};
	float voltage_sum[3] = {0.0, 0.0, 0.0};

	TIMER_UPDATE_DUTY_MT(TIM1->ARR / 2, TIM1->ARR / 2, TIM1->ARR / 2);

	// Start PWM on phase 1
	stop_pwm_hw();
	LL_TIM_SetOCMode(TIM1, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    LL_TIM_EnableCCxOutput(TIM1, LL_TIM_CHANNEL_CH1);      // CC1E = 1 主通道使能
    LL_TIM_EnableCCxNOutput(TIM1, LL_TIM_CHANNEL_CH1);    // CC1NE = 0 互补关闭

	osDelay(1);

	for (int i = 0;i < samples;i++) {
		current_sum[0] += m_motor_1.m_currents_adc[0];
		voltage_sum[0] += ADC_V_L1_VOLTS;
		osDelay(1);
	}



    stop_pwm_hw();

	m_motor_1.m_conf->foc_offsets_current[0] = current_sum[0] / samples;
	m_motor_1.m_conf->foc_offsets_current[1] = current_sum[1] / samples;
	m_motor_1.m_conf->foc_offsets_current[2] = current_sum[2] / samples;

	voltage_sum[0] /= samples;
	voltage_sum[1] /= samples;
	voltage_sum[2] /= samples;
	float v_avg = (voltage_sum[0] + voltage_sum[1] + voltage_sum[2]) / 3.0;

	m_motor_1.m_conf->foc_offsets_voltage[0] = voltage_sum[0] - v_avg;
	m_motor_1.m_conf->foc_offsets_voltage[1] = voltage_sum[1] - v_avg;
	m_motor_1.m_conf->foc_offsets_voltage[2] = voltage_sum[2] - v_avg;
}
