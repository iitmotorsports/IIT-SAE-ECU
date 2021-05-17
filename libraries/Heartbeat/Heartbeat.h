/**
 * @file Heartbeat.h
 * @author IR
 * @brief Make one ECU tell the other it is alive
 * @version 0.1
 * @date 2021-03-19
 * 
 * @copyright Copyright (c) 2021
 * 
 * @see Heartbeat for more info
 * @see Heartbeat.def for configuration of this module
 */

#ifndef __ECU_HEARTBEAT_H__
#define __ECU_HEARTBEAT_H__

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief A module used to both ensure a connection to both ECUs using CAN
 * and to periodically run callbacks.
 * 
 * Currently, Heartbeat::beginBeating() is run on the back ECU and Heartbeat::beginReceiving() is run on the front ECU.
 * 
 * Heartbeat::checkBeat() must be polled on the front ECU to log whether or not a heart beat is being detected
 * 
 * @see Heartbeat.def for configuration of this module
 */
namespace Heartbeat {

/**
 * @brief Function called each time the *heart* beats
 */
typedef void (*beatFunc)(void);

/**
 * @brief begin sending a beat signal at a set interval
 */
void beginBeating();

/**
 * @brief Set callback to receive beats over CAN
 */
void beginReceiving();

/**
 * @brief Poll if a beat has been received
 * 
 * @return 1 if the last beat is within allowed delay, 0 otherwise
 */
int checkBeat();

/**
 * @brief Add a callback to be run at each haertbeat
 * 
 * @param func Callback function
 */
void addCallback(beatFunc func);

} // namespace Heartbeat

#endif // __ECU_HEARTBEAT_H__