/**
 * \file vdml.c
 *
 * \brief VDML Devices Mutex Management.
 *
 * This file ensure thread saftey for operations on motors by maintaining
 * an array of RTOS Mutexes and implementing functions to take and give them.
 *
 * \copyright (c) 2017, Purdue University ACM SIGBots.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <errno.h>
#include <math.h>
#include <stdio.h>

#include "ifi/v5_api.h"
#include "kapi.h"
#include "pros/motors.h"
#include "vdml/registry.h"
#include "vdml/vdml.h"

int32_t motor_set_velocity(uint8_t port, const int16_t velocity) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorVelocitySet(device.device_info, velocity);
	return_port(port - 1, 1);
}

int32_t motor_set_voltage(uint8_t port, const int16_t voltage) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorVoltageSet(device.device_info, voltage);
	return_port(port - 1, 1);
}

int32_t motor_get_velocity(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int16_t rtn = vexDeviceMotorVelocityGet(device.device_info);
	return_port(port - 1, rtn);
}

double motor_get_actual_velocity(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	double rtn = vexDeviceMotorActualVelocityGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_get_direction(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int32_t rtn = vexDeviceMotorDirectionGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_set_current_limit(uint8_t port, const int32_t limit) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorCurrentLimitSet(device.device_info, limit);
	return_port(port - 1, 1);
}

int32_t motor_get_current_limit(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int32_t rtn = vexDeviceMotorCurrentLimitGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_set_voltage_limit(uint8_t port, const int32_t limit) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorVoltageLimitSet(device.device_info, limit);
	return_port(port - 1, 1);
}

int32_t motor_get_voltage_limit(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int32_t rtn = vexDeviceMotorVoltageLimitGet(device.device_info);
	return_port(rtn, port - 1);
}

int32_t motor_get_current(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int32_t rtn = vexDeviceMotorCurrentGet(device.device_info);
	return_port(port - 1, rtn);
}

double motor_get_voltage(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	double rtn = vexDeviceMotorVoltageGet(device.device_info);
	return_port(port - 1, rtn);
}

double motor_get_power(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	double rtn = vexDeviceMotorPowerGet(device.device_info);
	return_port(port - 1, rtn);
}

double motor_get_torque(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	double rtn = vexDeviceMotorTorqueGet(device.device_info);
	return_port(port - 1, rtn);
}

double motor_get_efficiency(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	double rtn = vexDeviceMotorEfficiencyGet(device.device_info);
	return_port(port - 1, rtn);
}

double motor_get_temperature(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	double rtn = vexDeviceMotorTemperatureGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_get_temp_limit_flag(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int rtn = vexDeviceMotorOverTempFlagGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_get_current_limit_flag(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int rtn = vexDeviceMotorCurrentLimitFlagGet(device.device_info);
	return_port(port - 1, rtn);
}

uint32_t motor_get_faults(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	uint32_t rtn = vexDeviceMotorFaultsGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_get_zero_velocity_flag(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int rtn = vexDeviceMotorZeroVelocityFlagGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_get_zero_position_flag(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int rtn = vexDeviceMotorZeroPositionFlagGet(device.device_info);
	return_port(port - 1, rtn);
}

uint32_t motor_get_flags(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	uint32_t rtn = vexDeviceMotorFlagsGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_set_reverse(uint8_t port, const bool reverse) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorReverseFlagSet(device.device_info, reverse);
	return_port(port - 1, 1);
}

int32_t motor_get_reverse(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int rtn = vexDeviceMotorReverseFlagGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_set_encoder_units(uint8_t port, const motor_encoder_units_e_t units) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorEncoderUnitsSet(device.device_info, units);
	return_port(port - 1, 1);
}

motor_encoder_units_e_t motor_get_encoder_units(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	V5MotorEncoderUnits rtn = vexDeviceMotorEncoderUnitsGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_set_brake_mode(uint8_t port, const motor_brake_mode_e_t mode) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorBrakeModeSet(device.device_info, mode);
	return_port(port - 1, 1);
}

motor_brake_mode_e_t motor_get_brake_mode(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	V5MotorBrakeMode rtn = vexDeviceMotorBrakeModeGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_set_position(uint8_t port, const double position) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorPositionSet(device.device_info, position);
	return_port(port - 1, 1);
}

double motor_get_position(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	double rtn = vexDeviceMotorPositionGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_get_raw_position(uint8_t port, uint32_t* const timestamp) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	int32_t rtn = vexDeviceMotorPositionRawGet(device.device_info, timestamp);
	return_port(port - 1, rtn);
}

int32_t motor_reset_position(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorPositionReset(device.device_info);
	return_port(port - 1, 1);
}

double motor_get_target(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	double rtn = vexDeviceMotorTargetGet(device.device_info);
	return_port(port - 1, rtn);
}

int32_t motor_set_absolute_target(uint8_t port, const double position, const int32_t velocity) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorAbsoluteTargetSet(device.device_info, position, velocity);
	return_port(port - 1, 1);
}

int32_t motor_set_relative_target(uint8_t port, const double position, const int32_t velocity) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorRelativeTargetSet(device.device_info, position, velocity);
	return_port(port - 1, 1);
}

int32_t motor_set_gearing(uint8_t port, const motor_gearset_e_t gearset) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	vexDeviceMotorGearingSet(device.device_info, gearset);
	return_port(port - 1, 1);
}

motor_gearset_e_t motor_get_gearing(uint8_t port) {
	claim_port(port - 1, E_DEVICE_MOTOR);
	V5MotorGearset rtn = vexDeviceMotorGearingGet(device.device_info);
	return_port(port - 1, rtn);
}