/**
 * @file Heartbeat.h
 * @author IR
 * @brief Make one ECU tell the other it is alive
 * @version 0.1
 * @date 2021-03-19
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ECU_HEARTBEAT_H__
#define __ECU_HEARTBEAT_H__

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief The interval the beat should target
 */
#define CONF_HEARTBEAT_INTERVAL_MICRO 150000
/**
 * @brief The allowed delay between beats
 */
#define CONF_HEARTBEAT_TIMEOUT_MILLI 50 // Allowable Delay

/**
 * @brief A module used to both ensure a connection to both ECUs using CAN
 * and to periodically run callbacks
 */
namespace Heartbeat {

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
 * @return int True if the last beat is within allowed delay
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