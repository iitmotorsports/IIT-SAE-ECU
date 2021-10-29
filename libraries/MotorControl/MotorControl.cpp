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
#include "Util.h"
#include "log.h"
#include "stdint.h"
#include "stdlib.h"

namespace MC {

static LOG_TAG ID = "MotorControl";

static int motorTorque[2] = {0, 0};

static bool beating = true;
static bool init = false;
static bool forward = true;

static int mc0T = 0;
static int mc1T = 0;

static int pAccum = 0;

static Canbus::Buffer MC0_RPM_Buffer(ADD_MC0_RPM);
static Canbus::Buffer MC1_RPM_Buffer(ADD_MC1_RPM);

static void beatFunc(void) {
    if (beating) {
        Canbus::sendData(ADD_MC0_CTRL);
        Canbus::sendData(ADD_MC1_CTRL);
    }
}

static int32_t motorSpeed(int motor = -1) {
    int16_t MC_Rpm_Val_0 = MC0_RPM_Buffer.getShort(2);                                 // Bytes 2-3 : Angular Velocity
    int16_t MC_Rpm_Val_1 = MC1_RPM_Buffer.getShort(2);                                 // Bytes 2-3 : Angular Velocity
    float MC_Spd_Val_0 = MC_Rpm_Val_0 * 2 * 3.1415926536 / 60 * CONF_CAR_WHEEL_RADIUS; // (RPM -> Rad/s) * Radius
    float MC_Spd_Val_1 = MC_Rpm_Val_1 * 2 * 3.1415926536 / 60 * CONF_CAR_WHEEL_RADIUS;
    switch (motor) {
    case 0:
        return MC_Spd_Val_0;
    case 1:
        return MC_Spd_Val_1;
    default:
        return (MC_Spd_Val_0 + MC_Spd_Val_1) / 2;
    }
}

static void torqueVector(int pedal, int brake, int steer) {
    pAccum = interpolate(10, pedal, (pedal - pAccum) / PINS_ANALOG_MAX, 15);
    // TODO: Add Torque vectoring algorithms
    motorTorque[0] = pAccum;
    motorTorque[1] = pAccum;
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

int sendSpeed(uint32_t MC_ADD, int speed, bool direction, bool enableBit) {
    if (beating) {
        Log.w(ID, "Unable to set torque, heartbeat is on");
        return 0;
    }
    int setSpeed = 0, torqueLimit = 50;
    if (speed != 0) {                                        // max analog should be what the max pedal readout is
        setSpeed = cMap(speed, 200, PINS_ANALOG_MAX, 0, 10); // separate func for negative vals (regen)
    }

    uint8_t *bytes = (uint8_t *)&setSpeed;
    uint8_t *limitBytes = (uint8_t *)&torqueLimit;
    Canbus::sendData(MC_ADD, 0, 0, bytes[0], bytes[1], direction, 0b101 * enableBit, limitBytes[0], limitBytes[1]);
    return setSpeed;
}

int sendTorque(uint32_t MC_ADD, int torque, bool direction, bool enableBit) { // NOTE: 0 (Clockwise = Reverse) 1 (Anticlockwise = Forward)
    if (beating) {
        Log.w(ID, "Unable to set torque, heartbeat is on");
        return 0;
    }
    int setTorque = 0;
    if (torque != 0) {                                          // max analog should be what the max pedal readout is
        setTorque = cMap(torque, 200, PINS_ANALOG_MAX, 0, 150); // separate func for negative vals (regen)
    }

    uint8_t *bytes = (uint8_t *)&setTorque;
    Canbus::sendData(MC_ADD, bytes[0], bytes[1], 0, 0, direction, enableBit);
    return setTorque;
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

int getLastTorqueValue(bool mc0) {
    return mc0 ? mc0T : mc1T;
}

int getLastPedalValue() {
    return pAccum;
}

void setTorque(int pedal, int brake, int steer) {
    torqueVector(pedal, brake, steer);
    // motorTorque[0] -= motorSpeed(0);
    // motorTorque[1] -= motorSpeed(1);
    mc0T = sendTorque(ADD_MC0_CTRL, motorTorque[0], forward, 1);
    mc1T = sendTorque(ADD_MC1_CTRL, motorTorque[1], forward, 1);
}

} // namespace MC

// @endcond