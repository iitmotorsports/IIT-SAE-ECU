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
#include "Pins.h"
#include "log.h"
#include "stdint.h"
#include "stdlib.h"

namespace MC {

static LOG_TAG ID = "MotorControl";
static bool beating = true;
static bool init = false;
static int MotorTorques[2] = {0, 0};

static void beatFunc(void) {
    if (beating) {
        Canbus::sendData(ADD_MC0_CTRL);
        Canbus::sendData(ADD_MC1_CTRL);
    }
}

void setup(void) {
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

void sendCommand(uint32_t MC_ADD, int torque, bool direction, bool enableBit) { // NOTE: 0 (Clockwise = Reverse) 1 (Anticlockwise = Forward)
    if (beating) {
        Log.w(ID, "Unable to set torque, heartbeat is on");
        return;
    }
    int percentTorque = 0;
    if (torque != 0) {
        percentTorque = constrain(map(torque, 200, PINS_ANALOG_MAX, 0, 300), 0, 300); // separate func for negative vals (regen)
    }
    uint8_t *bytes = (uint8_t *)&percentTorque;
    Canbus::sendData(MC_ADD, bytes[0], bytes[1], 0, 0, direction, enableBit);
}

static void torqueVector(int pedal, int brake, int steer) {
    // TODO: Add Torque vectoring algorithms
    MotorTorques[0] = pedal;
    MotorTorques[1] = pedal;
}

void setTorque(int pedal, int brake, int steer) {
    torqueVector(pedal, brake, steer);
    sendCommand(ADD_MC0_CTRL, MotorTorques[0], 0, 1);
    sendCommand(ADD_MC1_CTRL, MotorTorques[1], 1, 1);
}

} // namespace MC

// @endcond