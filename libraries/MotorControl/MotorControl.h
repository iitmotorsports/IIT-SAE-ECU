/**
 * @file MotorControl.h
 * @author IR
 * @brief Motor Controller module
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ECU_MOTORCONTROL_H__
#define __ECU_MOTORCONTROL_H__

#include "Canbus.h"
#include "Heartbeat.h"
#include "stdint.h"

namespace MC {

void initialize(void);

void clearFaults(void);

void enableMotorBeating(bool enable);

void sendCommand(uint32_t MC_ADD, int torque, bool direction, bool enableBit);

void setTorque(int torque);

} // namespace MC

// Add reverse control
#endif // __ECU_MOTORCONTROL_H__