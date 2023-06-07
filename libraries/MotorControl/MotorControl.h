/**
 * @file MotorControl.h
 * @author IR
 * @brief Motor Controller module
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2022
 * 
 * This module is a collection of functions that are used to ease the control of both Motor controllers over canbus.
 * 
 * The Motor controllers require a periodic message to be send when it is not being actively controlled,
 * This is mainly controlled with MC::enableMotorBeating()
 * 
 * It is important to be aware of delays in code as if the Motors controllers do not receive a value in time, they will fault.
 * 
 * Faults can, however, be cleared by using MC::clearFaults()
 * 
 * @see Faults.h for more info on viewing faults
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

/**
 * @brief The maximum speed value that the car can be moving before it switches between forwards and reverse
 */
#define CONF_MAXIMUM_SWITCHING_SPEED 5

/**
 * @brief Initialize MC heartbeat function and clear faults, if any
 */
void setup(void);

/**
 * @brief Only initialize MC buffers, called by MC::setup
 */
void setupBuffers(void);

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
 * @brief Send a raw torque command to a MC
 * 
 * @note `enableMotorBeating` must have been set to false before calling this function
 * 
 * @param MC_ADD The specific motor controller address
 * @param torque The torque to set the motor at
 * @param enableBit whether the motor should be enabled
 */
void sendTorque(uint32_t MC_ADD, int torque, bool enableBit);

/**
 * @brief Get the last torque percent value sent to a MC
 * 
 * @param mc0 return the value for MC0, otherwise MC1
 * @return int the last torque percent value that was sent
 */
int getLastTorqueValue(bool mc0);

/**
 * @brief Get the last pedal value that was internally used
 * 
 * @return int the last pedal value that was internally used
 */
int getLastPedalValue(void);

/**
 * @brief Get the last brake value that was internally used
 * 
 * @return int the last brake value that was internally used
 */
int getLastBrakeValue();

/**
 * @brief Get the last steer value that was internally used
 * 
 * @return int the last steer value that was internally used
 */
int getLastSteerValue();

/**
 * @brief Get the avg motorspeed
 * 
 * @param motor 0 or 1 for either motor or nothing for 
 * @return int32_t 
 */
int32_t motorSpeed(int motor = -1);

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

#endif // __ECU_MOTORCONTROL_H__