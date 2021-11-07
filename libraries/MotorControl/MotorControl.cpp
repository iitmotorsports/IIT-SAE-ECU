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
#include "SerialVar.h"
#include "Util.h"
#include "log.h"
#include "stdint.h"
#include "stdlib.h"

namespace MC {

#define MAX_TORQUE 500.0
#define NORM_VAL (double)PINS_ANALOG_MAX

#define PEDAL_MIN 200.0
#define PEDAL_MAX 1300.0

// #define PEDAL_MIN 0.0
// #define PEDAL_MAX (double)PINS_ANALOG_HIGH

// TODO: get input value ranges

#define BRAKE_MIN 0.0
#define BRAKE_MAX (double)PINS_ANALOG_HIGH

#define STEER_MIN 0.0
#define STEER_MAX (double)PINS_ANALOG_HIGH

static LOG_TAG ID = "MotorControl";

static int motorTorque[2] = {0, 0};

static bool beating = true;
static bool init = false;
static bool forward = true;

static double pAccum = 0, bAccum = 0, sAccum = 0;

float TVAggression = 0.8f;

// SerialVar::SerialVarObj TVAggressiona = SerialVar::variables[SERIALVAR_TORQUE_VECTORING_AGGRESSION];

static Canbus::Buffer MC0_RPM_Buffer(ADD_MC0_RPM);
static Canbus::Buffer MC1_RPM_Buffer(ADD_MC1_RPM);

static void beatFunc(void) {
    if (beating) {
        Canbus::sendData(ADD_MC0_CTRL);
        Canbus::sendData(ADD_MC1_CTRL);
    }
}

int32_t motorSpeed(int motor = -1) {                                                   // FIXME: Motor might be sending negative speed?
    int16_t MC_Rpm_Val_0 = abs(MC0_RPM_Buffer.getShort(2));                            // Bytes 2-3 : Angular Velocity
    int16_t MC_Rpm_Val_1 = abs(MC1_RPM_Buffer.getShort(2));                            // Bytes 2-3 : Angular Velocity
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

static void normalizeInput(double *pedal, double *brake, double *steer) { // TODO: GET THIS OUT OF HERE! Does not belong here
    pAccum = EMAvg(pAccum, cMap(*pedal, PEDAL_MIN, PEDAL_MAX, 0.0, NORM_VAL), 32);
    bAccum = EMAvg(bAccum, cMap(*brake, BRAKE_MIN, BRAKE_MAX, 0.0, NORM_VAL), 16);
    sAccum = EMAvg(sAccum, cMap(*steer, STEER_MIN, STEER_MAX, 0.0, NORM_VAL), 8);

    *pedal = pAccum;
    *brake = bAccum;
    *steer = cMap(sAccum, 0.0, NORM_VAL, -PI / 2, PI / 2);
}

static void torqueVector(int pedal, int brake, int steer) {
    double _pedal = pedal, _brake = brake, _steer = steer;

    normalizeInput(&_pedal, &_brake, &_steer);

    // TV V1
    // if (_steer <= 0) {
    //     motorTorque[0] = cMap(_pedal, 0.0, NORM_VAL, 0.0, MAX_TORQUE);
    //     motorTorque[1] = motorTorque[0] * clamp(pow(cos(TVAggression * _steer), 5), 0, 1);
    // } else {
    //     motorTorque[1] = cMap(_pedal, 0.0, NORM_VAL, 0.0, MAX_TORQUE);
    //     motorTorque[0] = motorTorque[1] * clamp(pow(cos(TVAggression * _steer), 5), 0, 1);
    // }
    motorTorque[0] = cMap(_pedal, 0.0, NORM_VAL, 0.0, MAX_TORQUE);
    motorTorque[1] = motorTorque[0];
}

void setup(void) {
    MC0_RPM_Buffer.init();
    MC1_RPM_Buffer.init();
#if CONF_ECU_POSITION == BACK_ECU
    if (!init)
        Heartbeat::addCallback(beatFunc);
    clearFaults();
    init = true;
    beating = true;
#endif
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

void setDirection(bool runForward) {
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

void setTorque(int pedal, int brake, int steer) {
    if (forward && pedal >= 0) // Regen brake
        torqueVector(pedal, brake, steer);
    sendTorque(ADD_MC1_CTRL, motorTorque[1], forward, 1);
    sendTorque(ADD_MC0_CTRL, motorTorque[0], forward, 1);
}

} // namespace MC

// @endcond