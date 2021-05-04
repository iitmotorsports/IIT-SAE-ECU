/**
 * @file MotorControl.cpp
 * @author IR
 * @brief MotorControl Source File
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021
 * 
 */

// @cond

#include "MotorControl.h"
#include "MotorControl.def"
#include "Pins.h"
#include "log.h"
#include "stdint.h"
#include "stdlib.h"

namespace MC {

static LOG_TAG ID = "MotorControl";
static bool beating = true;
static bool init = false;

static void beatFunc(void) {
    if (beating) {
        Canbus::sendData(ADD_MC0_CTRL);
        Canbus::sendData(ADD_MC1_CTRL);
    }
}

void initialize(void) {
    if (!init)
        Heartbeat::addCallback(beatFunc);
    clearFaults();
    init = true;
    beating = true;
}

void clearFaults(void) {
    Canbus::sendData(ADD_MC0_CLEAR, 20, 0, 1);
    Canbus::sendData(ADD_MC1_CLEAR, 20, 0, 1);
    Canbus::sendData(ADD_MC0_CTRL);
    Canbus::sendData(ADD_MC1_CTRL);
}

void enableMotorBeating(bool enable) {
    beating = enable;
}

void sendCommand(uint32_t MC_ADD, int torque, bool direction, bool enableBit) {
    int percentTorque = constrain(map(torque, 0, PINS_ANALOG_MAX, 0, 10), 0, 10); // separate func for negative vals (regen)
    uint8_t *bytes = (uint8_t *)&percentTorque;
    Canbus::sendData(MC_ADD, bytes[0], bytes[1], 0, 0, direction, enableBit);
}

void setTorque(int torque) {
    if (beating) {
        Log.w(ID, "Unable to set torque, heartbeat is on");
        return;
    }
    sendCommand(ADD_MC0_CTRL, torque, 0, 1);
    sendCommand(ADD_MC0_CTRL, torque, 1, 1);
}

} // namespace MC

// @endcond