/**
 * @file MotorControl.cpp
 * @author IR
 * @brief MotorControl Source File
 * @version 0.1
 * @date 2021-05-04
 *
 * @copyright Copyright (c) 2022
 *
 */

// @cond

#include <stdint.h>
#include <stdlib.h>

#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "MotorControl.def"
#include "MotorControl.h"
#include "Pins.h"
#include "Util.h"
#include "log.h"

namespace MC {

#define NORM_VAL (double)PINS_ANALOG_MAX
#define MAX_TORQUE CONF_MAX_TORQUE

#define PEDAL_MIN CONF_PEDAL_MIN
#define PEDAL_MAX CONF_PEDAL_MAX

#define BRAKE_MIN CONF_BRAKE_MIN
#define BRAKE_MAX CONF_BRAKE_MAX

#define STEER_MIN CONF_STEER_MIN
#define STEER_MAX CONF_STEER_MAX

static LOG_TAG ID = "MotorControl";

static int motorTorque[2] = {0, 0};

static bool beating = true;
static bool init = false;
static bool forward = true;

static double pAccum = 0, bAccum = 0, sAccum = 0;

static Canbus::Buffer *MC0_RPM_Buffer = Canbus::getBuffer(ADD_MC0_RPM);
static Canbus::Buffer *MC1_RPM_Buffer = Canbus::getBuffer(ADD_MC1_RPM);

static void beatFunc(void) {
    if (beating) {
        Canbus::sendData(ADD_MC0_CTRL);
        Canbus::sendData(ADD_MC1_CTRL);
    }
}

constexpr float c = 2 * 3.1415926536 * 9;

static int32_t lastspeed;

int32_t motorSpeed(int motor) {
    Canbus::Buffer::lock MC0_lock = MC0_RPM_Buffer->get_lock(Canbus::DEFAULT_TIMEOUT);
    Canbus::Buffer::lock MC1_lock = MC1_RPM_Buffer->get_lock(Canbus::DEFAULT_TIMEOUT);
    if (!MC0_lock.locked || !MC1_lock.locked) {
        Log.w(ID, "Unable to lock buffers for speed", lastspeed);
        return lastspeed;
    }
    int16_t MC_Rpm_Val_0 = -MC0_RPM_Buffer->getShort(2); // Bytes 2-3 : Angular Velocity // NOTE: motor is negative?
    int16_t MC_Rpm_Val_1 = MC1_RPM_Buffer->getShort(2);  // Bytes 2-3 : Angular Velocity
    float MC_Spd_Val_0 = (float)(((MC_Rpm_Val_0 / CONF_CAR_GEAR_RATIO) * c) * 60) / 63360;
    float MC_Spd_Val_1 = (float)(((MC_Rpm_Val_1 / CONF_CAR_GEAR_RATIO) * c) * 60) / 63360;

    switch (motor) {
    case 0:
        return lastspeed = MC_Spd_Val_0;
    case 1:
        return lastspeed = MC_Spd_Val_1;
    default:
        return lastspeed = (MC_Spd_Val_0 + MC_Spd_Val_1) / 2;
    }
}

static void normalizeInput(double *pedal, double *brake, double *steer) { // TODO: GET THIS OUT OF HERE! Does not belong here
    pAccum = EMAvg(pAccum, cMap(*pedal, PEDAL_MIN, PEDAL_MAX, 0.0, NORM_VAL), 8);
    bAccum = EMAvg(bAccum, cMap(*brake, BRAKE_MIN, BRAKE_MAX, 0.0, NORM_VAL), 8);
    sAccum = EMAvg(sAccum, cMap(*steer, STEER_MIN, STEER_MAX, 0.0, NORM_VAL), 8);

    *pedal = pAccum;
    *brake = bAccum;
    *steer = cMap(sAccum, 0.0, NORM_VAL, -PI / 9, PI / 9);
}

static void torqueVector(int pedal, int brake, int steer) {

    double _pedal = pedal, _brake = brake, _steer = steer;

    normalizeInput(&_pedal, &_brake, &_steer);

    float TVAggression = (float)Pins::getCanPinValue(PINS_INTERNAL_TVAGG) / 10000.0f;

    Log.d(ID, "Aggression Val x1000:", TVAggression * 1000, true);

    // No TV

    // TODO: Test Torque Vectoring and Regenerative braking

    if (_pedal < 0) { // FIXME: Sensor on car flipped?
        // _pedal = -_pedal;
        // motorTorque[0] = -cMap(_pedal, 0.0, NORM_VAL, 0.0, MAX_TORQUE);
        // motorTorque[1] = -cMap(_pedal, 0.0, NORM_VAL, 0.0, MAX_TORQUE);
    } else {
        motorTorque[0] = cMap(_pedal, 0.0, NORM_VAL, 0.0, MAX_TORQUE);
        motorTorque[1] = cMap(_pedal, 0.0, NORM_VAL, 0.0, MAX_TORQUE);

        // Flipped sensor?

        // TV V1
        // if (_steer > 0) {
        //     motorTorque[0] = cMap(_pedal, 0.0, NORM_VAL, 0.0, MAX_TORQUE);
        //     motorTorque[1] = motorTorque[0] * clamp(pow(cos(TVAggression * _steer), 5), 0, 1);
        // } else {
        //     motorTorque[1] = cMap(_pedal, 0.0, NORM_VAL, 0.0, MAX_TORQUE);
        //     motorTorque[0] = motorTorque[1] * clamp(pow(cos(TVAggression * _steer), 5), 0, 1);
        // }
    }

    // motorTorque[0] = cMap(_pedal, 0.0, NORM_VAL, 0.0, MAX_TORQUE);
    // motorTorque[1] = motorTorque[0];
}

void setup(void) { // IMPROVE: receive message from MCs, not just send
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

void sendTorque(uint32_t MC_ADD, int torque, bool direction, bool enableBit) { // NOTE: 0 (Clockwise = Reverse) 1 (Anticlockwise = Forward)
    if (beating) {
        Log.w(ID, "Unable to set torque, heartbeat is on");
        return;
    }
    uint8_t *bytes = (uint8_t *)&torque;
    Canbus::sendData(MC_ADD, bytes[0], bytes[1], 0, 0, direction, enableBit);
}

bool isForward(void) {
    return forward;
}

void setDirection(bool runForward) { // FIXME: Inverter must be switched off before switching direction, else fault must be cleared and inverter started again
    if (motorSpeed() <= CONF_MAXIMUM_SWITCHING_SPEED) {
        Log.w(ID, "Switching direction");
        forward = runForward;
    } else {
        Log.e(ID, "Unable to switch direction, car is moving too much");
    }
}

int getLastTorqueValue(bool mc0) {
    return mc0 ? motorTorque[0] : motorTorque[1];
}

int getLastPedalValue() {
    return pAccum;
}

int getLastBrakeValue() {
    return (int)bAccum;
}

int getLastSteerValue() {
    return cMap(sAccum, 0.0, NORM_VAL, -PI / 9, PI / 9);
}

void setTorque(int pedal, int brake, int steer) {
    torqueVector(pedal, brake, steer);
    sendTorque(ADD_MC1_CTRL, motorTorque[1], forward, 1);
    sendTorque(ADD_MC0_CTRL, motorTorque[0], forward, 1);
}

} // namespace MC

// @endcond