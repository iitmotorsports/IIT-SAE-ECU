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
#include "Pins.h"

namespace Heartbeat {
static IntervalTimer canbusPinUpdate;
static elapsedMillis lastBeat;
static uint lastTime = 0;

static LOG_TAG ID = "HeartBeat";

static bool beatMCs = true;

static void toggleLED() {
    static bool on = false;
    on = !on;
    Pins::setPinValue(PINS_BOTH_LED, on);
}

static void beat() {
    Canbus::sendData(ADD_HEART);
    toggleLED();
    if (beatMCs) {
        Canbus::sendData(ADD_MC0_CTRL, 0, 0, 0, 0, 0, 0, 0, 0);
        Canbus::sendData(ADD_MC1_CTRL, 0, 0, 0, 0, 1, 0, 0, 0);
    }
}

void enableMotorBeating(bool enable) {
    beatMCs = enable;
}

void beginBeating() {
    canbusPinUpdate.priority(1);
    beatMCs = true;
    canbusPinUpdate.begin(beat, CONF_HEARTBEAT_INTERVAL_MICRO);
    for (size_t i = 0; i < 4; i++) {
        Canbus::sendData(ADD_MC0_CLEAR, 20, 0, 1); // NOTE: based off documentation example, MCs are little endian
        Canbus::sendData(ADD_MC1_CLEAR, 20, 0, 1);
        delay(50);
    }
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

} // namespace Heartbeat