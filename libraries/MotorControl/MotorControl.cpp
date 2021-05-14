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
#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "Pins.h"
#include "log.h"
#include "stdint.h"
#include "stdlib.h"

namespace MC {

static LOG_TAG ID = "MotorControl";

static int motorTorque[2] = {0, 0};

static bool beating = true;
static bool init = false;
static bool forward = true;

static Canbus::Buffer MC0_RPM_Buffer(ADD_MC0_RPM);
static Canbus::Buffer MC1_RPM_Buffer(ADD_MC1_RPM);

static void beatFunc(void) {
    if (beating) {
        Canbus::sendData(ADD_MC0_CTRL);
        Canbus::sendData(ADD_MC1_CTRL);
    }
}

static int32_t motorSpeed() {
    int16_t MC_Rpm_Val_0 = MC0_RPM_Buffer.getShort(2); // Bytes 2-3 : Angular Velocity
    int16_t MC_Rpm_Val_1 = MC1_RPM_Buffer.getShort(2); // Bytes 2-3 : Angular Velocity
    float MC_Spd_Val_0 = CONF_CAR_WHEEL_RADIUS * 2 * 3.1415926536 / 60 * MC_Rpm_Val_0;
    float MC_Spd_Val_1 = CONF_CAR_WHEEL_RADIUS * 2 * 3.1415926536 / 60 * MC_Rpm_Val_1;
    return (MC_Spd_Val_0 + MC_Spd_Val_1) / 2;
}

static void torqueVector(int pedal, int brake, int steer) {
    // TODO: Add Torque vectoring algorithms
    motorTorque[0] = pedal;
    motorTorque[1] = pedal;
}

void setup(void) {
    if (!init)
        Heartbeat::addCallback(beatFunc);
    MC0_RPM_Buffer.init();
    MC1_RPM_Buffer.init();
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

bool isForward(void) {
    return forward;
}

void setDirection(bool runForward) {
    if (motorSpeed() <= CONF_MAXIMUM_SWITCHING_SPEED) {
        Log.w(ID, "Switching direction");
        forward = runForward;
    } else {
        Log.e(ID, "Unable to switch direction, car is moving too much");
    }
}

void setTorque(int pedal, int brake, int steer) {
    torqueVector(pedal, brake, steer);
    sendCommand(ADD_MC0_CTRL, motorTorque[0], !forward, 1);
    sendCommand(ADD_MC1_CTRL, motorTorque[1], forward, 1);
}

} // namespace MC

// @endcond