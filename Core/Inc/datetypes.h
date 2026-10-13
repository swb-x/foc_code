#ifndef __DATETYPES_H__
#define __DATETYPES_H__

#include <stdbool.h>

typedef enum {
	FAULT_CODE_NONE = 0,
	FAULT_CODE_OVER_VOLTAGE,
	FAULT_CODE_UNDER_VOLTAGE,
	FAULT_CODE_DRV8323,
	FAULT_CODE_OVER_TEMP,
	FAULT_CODE_OVER_CURRENT_U,
	FAULT_CODE_OVER_CURRENT_V,
	FAULT_CODE_OVER_CURRENT_W,
	FAULT_CODE_PHASE_FILTER,
	FAULT_CODE_ENCODER_FAULT,
	FAULT_CODE_OVERSPEED,
	FAULT_CODE_UNDERSPEED,
	FAULT_CODE_ABS_OVERSPEED
} mc_fault_code;

typedef struct {

	float foc_offsets_current[3];
	float foc_offsets_voltage[3];

	// Speed PID
	float s_pid_kp;
	float s_pid_ki;
	float s_pid_kd;
	float s_pid_kd_filter;
	float s_pid_min_erpm;
	bool s_pid_allow_braking;
	float s_pid_ramp_erpms_s;

	// Pos PID
	float p_pid_kp;
	float p_pid_ki;
	float p_pid_kd;
	float p_pid_kd_proc;
	float p_pid_kd_filter;
	float p_pid_ang_div;
	float p_pid_gain_dec_angle;
	float p_pid_offset;

	// Current controller
	float cc_startup_boost_duty;
	float cc_min_current;
	float cc_gain;
	float cc_ramp_step_max;

	// Protect from flash corruption.
	uint16_t crc;
} mc_configuration;


#endif