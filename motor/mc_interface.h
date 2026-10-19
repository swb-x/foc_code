#ifndef __MC_INTERFACE_H__
#define __MC_INTERFACE_H__

#include "datetypes.h"

typedef struct {
	mc_configuration m_conf;
	mc_fault_code m_fault_now;
	float m_motor_current_sum;
	float m_motor_current_iterations;
	float m_motor_id_sum;
	float m_motor_iq_sum;
	float m_motor_id_iterations;
	float m_motor_iq_iterations;
	float m_motor_vd_sum;
	float m_motor_vq_sum;
	float m_motor_vd_iterations;
	float m_motor_vq_iterations;
	float m_amp_seconds;
	float m_amp_seconds_charged;
	float m_watt_seconds;
	float m_watt_seconds_charged;
	float m_position_set;
	float m_temp_fet;
	float m_temp_motor;
	float m_gate_driver_voltage;
	float m_motor_current_unbalance;
	float m_motor_current_unbalance_error_rate;
	float m_input_voltage_filtered;
	float m_input_voltage_filtered_slower;
	float m_temp_override;
	float m_i_in_filter;

	// Backup data counters
	uint64_t m_odometer_last;
	uint64_t m_runtime_last;
} motor_if_state_t;

#endif

