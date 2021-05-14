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

/**
 * @brief Methods used to more easily interface with RMS Motor controllers over CAN bus
 * 
 */
namespace MC {

#define CONF_MAXIMUM_SWITCHING_SPEED 5

/**
 * @brief Initialize MC heartbeat function and clear faults, if any
 */
void setup(void);

/**
 * @brief Clear MC faults by sending a clear fault command
 */
void clearFaults(void);

/**
 * @brief Start MC heartbeat function, establishing a connection with the MCs
 * 
 * @param enable 
 */
void enableMotorBeating(bool enable);

/**
 * @brief Send a raw control command to a MC
 * 
 * @note `enableMotorBeating` must have been set to false before calling this function
 * 
 * @param MC_ADD The specific motor controller address
 * @param torque The torque to set the motor at
 * @param direction direction of the motor
 * @param enableBit whether the motor should be enabled
 */
void sendCommand(uint32_t MC_ADD, int torque, bool direction, bool enableBit);

/**
 * @brief Set the direction of the motors
 * 
 * @param runForward whether the motors should spin forwards
 */
void setDirection(bool runForward);

/**
 * @brief Get whether the motors will spin forward
 * 
 * @return true The motors will spin forward
 * @return false The motors will spin in reverse
 */
bool isForward(void);

/**
 * @brief Calculate and set the torque of both MCs
 * 
 * @note `enableMotorBeating` must have been set to false before calling this function
 * 
 * @param pedal The raw value of the pedal
 * @param brake The raw value of the brake
 * @param steer The raw value of the steering wheel
 */
void setTorque(int pedal, int brake, int steer);

} // namespace MC

// Add reverse control
#endif // __ECU_MOTORCONTROL_H__