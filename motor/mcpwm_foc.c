#include "mcpwm_foc.h"
#include "foc_math.h"
#include "cmsis_os.h"
#include "stm32g4xx_ll_gpio.h"

static volatile motor_all_state_t m_motor;

static void stop_pwm_hw(void);
static void start_pwm_hw(void);

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
    // LL_GPIO_SetOutputPin(ENABLE_GPIO_Port, ENABLE_Pin);
    // osDelay(2);

    /* 5. 最后开 MOE，六路 PWM 同时上线 */
    LL_TIM_EnableAllOutputs(TIM1);

    printf("PWM 50%% output ON\r\n");
}

/* PWM输出 */
void Motor_PWM_Start(void)
{
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

void mcpwm_foc_dc_cal(void)
{
    const float samples = 1000.0;
	float current_sum[3] = {0.0, 0.0, 0.0};
	float voltage_sum[3] = {0.0, 0.0, 0.0};

	TIMER_UPDATE_DUTY_MT(TIM1->ARR / 2, TIM1->ARR / 2, TIM1->ARR / 2);

	// Start PWM on phase 1
	stop_pwm_hw();
	LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);      // CC1E = 1 主通道使能
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1N);   
    LL_TIM_GenerateEvent_COM(TIM1);

	osDelay(1);

	for (int i = 0;i < samples;i++) {
		current_sum[0] += m_motor.m_currents_adc[0];
		// voltage_sum[0] += ADC_V_L1_VOLTS;
		osDelay(1);
	}
	// Start PWM on phase 2
	stop_pwm_hw();
	LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);      // CC2E = 1 主通道使能
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2N);    
    LL_TIM_GenerateEvent_COM(TIM1);

	osDelay(1);

	for (int i = 0;i < samples;i++) {
		current_sum[1] += m_motor.m_currents_adc[1];
		// voltage_sum[1] += ADC_V_L2_VOLTS;
		osDelay(1);
	}
	// Start PWM on phase 3
	stop_pwm_hw();
	LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);      // CC3E = 1 主通道使能
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3N);    
    LL_TIM_GenerateEvent_COM(TIM1);

	osDelay(1);

	for (int i = 0;i < samples;i++) {
		current_sum[2] += m_motor.m_currents_adc[2];
		// voltage_sum[2] += ADC_V_L3_VOLTS;
		osDelay(1);
	}

    stop_pwm_hw();

	// m_motor.m_conf->foc_offsets_current[0] = current_sum[0] / samples;
	// m_motor.m_conf->foc_offsets_current[1] = current_sum[1] / samples;
	// m_motor.m_conf->foc_offsets_current[2] = current_sum[2] / samples;

	// voltage_sum[0] /= samples;
	// voltage_sum[1] /= samples;
	// voltage_sum[2] /= samples;
	// float v_avg = (voltage_sum[0] + voltage_sum[1] + voltage_sum[2]) / 3.0;

	// m_motor_1.m_conf->foc_offsets_voltage[0] = voltage_sum[0] - v_avg;
	// m_motor_1.m_conf->foc_offsets_voltage[1] = voltage_sum[1] - v_avg;
	// m_motor_1.m_conf->foc_offsets_voltage[2] = voltage_sum[2] - v_avg;
}

static void stop_pwm_hw(void){
    //======== TIM1 CH1 =========
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_FORCED_INACTIVE);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);      // CC1E = 1 主通道使能
    LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH1N);    // CC1NE = 0 互补关闭

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
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);      // CC1E = 1 主通道使能
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




/***************************************************init program **************************************************/
void drv8323_init(void);

void mc_config_init(void)
{
    // 初始化电机参数
    memset((void* )&m_motor, 0, sizeof(motor_all_state_t));
    
    // 初始化DRV8323
    drv8323_init();

	/* DC校准电压电流 */
    mcpwm_foc_dc_cal();
}


void drv8323_init(void)
{
    /* 失调电压校准 */
    LL_GPIO_SetOutputPin(CAL_GPIO_Port, CAL_Pin);
    osDelay(1); // 延时1ms 等待校准完成

}