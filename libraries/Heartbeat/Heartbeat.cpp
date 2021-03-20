/**
 * @file Heartbeat.cpp
 * @author IR
 * @brief Heartbeat source file
 * @version 0.1
 * @date 2021-03-19
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "Heartbeat.h"
#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "Log.h"

namespace Heartbeat {
static IntervalTimer canbusPinUpdate;
static elapsedMillis lastBeat;

static LOG_TAG ID = "HeartBeat";

static void beat() {
    Canbus::sendData(ADD_HEART);
}

void beginBeating() {
    canbusPinUpdate.priority(255);
    canbusPinUpdate.begin(beat, CONF_HEARTBEAT_INTERVAL_MICRO);
}

static void receiveBeat(uint32_t, volatile uint8_t *) {
    Log.w(ID, "Beat", lastBeat);
    lastBeat = 0;
}

void beginReceiving() {
    Canbus::addCallback(ADD_HEART, receiveBeat);
}

void checkBeat() {
    if (lastBeat > (CONF_HEARTBEAT_INTERVAL_MICRO / 1000) + CONF_HEARTBEAT_TIMEOUT_MILLI) {
        Log.w(ID, "Heartbeat is taking too long", lastBeat);
    }
}

} // namespace Heartbeat