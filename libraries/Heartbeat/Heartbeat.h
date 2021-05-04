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

#define CONF_HEARTBEAT_INTERVAL_MICRO 200000 // Every half second

#define CONF_HEARTBEAT_TIMEOUT_MILLI 50 // Allowable Delay

namespace Heartbeat {

typedef void (*beatFunc)(void);

void enableMotorBeating(bool enable);

void beginBeating();

void beginReceiving();

int checkBeat();

void addCallback(beatFunc func);

} // namespace Heartbeat

#endif // __ECU_HEARTBEAT_H__