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
//@cond

#include "Heartbeat.h"
#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "Log.h"
#include "Pins.h"
#include "stdint.h"
#include "stdlib.h"
#include "Heartbeat.def"
#include <set>

namespace Heartbeat {
static IntervalTimer canbusPinUpdate;
static elapsedMillis lastBeat;
static uint lastTime = 0;

static LOG_TAG ID = "HeartBeat";

static std::set<beatFunc> funcs;

static void toggleLED() {
    static bool on = false;
    on = !on;
    Pins::setPinValue(PINS_BOTH_LED, on);
}

static void beat() {
    Canbus::sendData(ADD_HEART);
    toggleLED();

    for (auto f : funcs) {
        f();
    }
}

void beginBeating() {
    canbusPinUpdate.priority(10);
    canbusPinUpdate.begin(beat, CONF_HEARTBEAT_INTERVAL_MICRO);
}

static void receiveBeat(uint32_t, volatile uint8_t *) {
    lastTime = lastBeat;
    lastBeat = 0;
    toggleLED();
}

void beginReceiving() {
    Canbus::addCallback(ADD_HEART, receiveBeat);
}

int checkBeat() {
    if (lastBeat > (CONF_HEARTBEAT_INTERVAL_MICRO / 1000) + CONF_HEARTBEAT_TIMEOUT_MILLI) {
        Log.w(ID, "Heartbeat is taking too long", lastBeat);
        return 0;
    } else {
        Log(ID, "Beat", lastTime);
        return 1;
    }
}

void addCallback(beatFunc func) {
    funcs.insert(func);
}

} // namespace Heartbeat
//@endcond