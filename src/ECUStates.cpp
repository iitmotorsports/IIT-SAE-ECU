#include "ECUStates.hpp"
#include "AeroServo.h"
#include "ECUGlobalConfig.h"
#include "Echo.h"
#include "Faults.h"
#include "Heartbeat.h"
#include "Log.h"
#include "Mirror.h"
#include "MotorControl.h"
#include "Util.h"

static bool FaultCheck() { // NOTE: Will only return true if hardfault occurs
    if (Fault::softFault())
        Fault::logFault();
    if (Fault::hardFault())
        return true;
    return false;
}

static void updateFaultLights() {
    static int bms, imd, bms_l, imd_l = 0;
    if ((bms = Pins::getPinValue(PINS_BACK_BMS_FAULT)) != bms_l || (imd = Pins::getPinValue(PINS_BACK_IMD_FAULT)) != imd_l) {
#ifdef CONF_ECU_DEBUG
        Log.d("FaultLights", "Updating Lights");
#endif
        Pins::setInternalValue(PINS_INTERNAL_BMS_FAULT, bms);
        Pins::setInternalValue(PINS_INTERNAL_IMD_FAULT, imd);
        bms_l = bms;
        imd_l = imd;
    }
}

State::State_t *ECUStates::Initialize_State::run(void) {
    Log.i(ID, "Teensy 3.6 SAE BACK ECU Initalizing");
    Log.i(ID, "Setup canbus");
    Canbus::setup(); // allocate and organize addresses
    Log.i(ID, "Initialize pins");
    Pins::initialize(); // setup predefined pins
    Log.i(ID, "Waiting for sync");
    while (!Pins::getCanPinValue(PINS_INTERNAL_SYNC)) {
        Canbus::sendData(ADD_MC0_CTRL);
        Canbus::sendData(ADD_MC1_CTRL);
        delay(100);
    }
    Log.i(ID, "Setup faults");
    Fault::setup(); // load all buffers
    Aero::setup();
    MC::setup();
#ifdef CONF_ECU_DEBUG
    Mirror::setup();
    Echo::setup();
#endif
    Heartbeat::addCallback(updateFaultLights); // IMPROVE: don't just periodically check if leds are on
    Heartbeat::beginBeating();

    // Front teensy should know when to blink start light
    Log.d(ID, "Checking for Inital fault");

    // NOTE: IMD Fault does not matter in initalizing state
    if (!Pins::getPinValue(PINS_BACK_IMD_FAULT) && FaultCheck()) {
        Log.e(ID, "Inital fault check tripped");
        return &ECUStates::FaultState;
    }

    // TSV
    Log.i(ID, "Waiting for shutdown signal");

    elapsedMillis shutdownBounce;

    while (true) {
        if (Pins::getPinValue(PINS_BACK_SHUTDOWN_SIGNAL)) {
            if (shutdownBounce > 50)
                break;
        } else {
            shutdownBounce = 0;
        }
    }

    Log.i(ID, "Shutdown signal received");

    Log.d(ID, "Finshed Setup");
    return &ECUStates::PreCharge_State;
};

State::State_t *ECUStates::PreCharge_State::PreCharFault(void) {
    Log.w(ID, "Opening Air1, Air2 and Precharge Relay");
    Pins::setPinValue(PINS_BACK_AIR1, LOW);
    Pins::setPinValue(PINS_BACK_AIR2, LOW);
    Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, LOW);
    Log.e(ID, "Precharge Faulted during precharge");
    return &ECUStates::FaultState;
}

bool ECUStates::PreCharge_State::voltageCheck() {
    int16_t BMSVolt = BMS_DATA_Buffer.getShort(2) / 10; // Byte 2-3: Pack Instant Voltage
    int16_t MC0Volt = MC0_VOLT_Buffer.getShort(0) / 10; // Bytes 0-1: DC BUS MC Voltage
    int16_t MC1Volt = MC1_VOLT_Buffer.getShort(0) / 10; // Bytes 0-1: DC BUS MC Voltage

    return 0.62 * BMSVolt <= (MC0Volt + MC1Volt) / 2;
}

void ECUStates::PreCharge_State::getBuffers() {
    BMS_DATA_Buffer.init();
    MC1_VOLT_Buffer.init();
    MC0_VOLT_Buffer.init();
};

State::State_t *ECUStates::PreCharge_State::run(void) { // NOTE: Low = Closed, High = Open
    Log.i(ID, "Loading Buffers");
    getBuffers();
    Log.i(ID, "Precharge running");

    if (FaultCheck()) {
        Log.e(ID, "Precharge Faulted before precharge");
        return &ECUStates::FaultState;
    }

    // NOTE: Assuming Air2 is correct
    Log.w(ID, "Closing Air2 and Precharge Relay and opening Air1");
    Pins::setPinValue(PINS_BACK_AIR2, PINS_ANALOG_HIGH);
    Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, PINS_ANALOG_HIGH);
    Pins::setPinValue(PINS_BACK_AIR1, LOW);

    if (FaultCheck()) {
        return PreCharFault();
    }

    elapsedMillis timeElapsed;

    Log.d(ID, "Running precharge loop");

    while (timeElapsed <= 10000) {
        if (timeElapsed >= 1000 && voltageCheck()) { // NOTE: will always pass if submodules are disconnected from CAN net
            Log.w(ID, "Opening precharge relay");
            Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, LOW);
            Log.i(ID, "Precharge Finished, closing Air1");
            Pins::setPinValue(PINS_BACK_AIR1, PINS_ANALOG_HIGH);
            return &ECUStates::Idle_State;
        }
    }

    Log.e(ID, "Precharge timed out");
    return PreCharFault();
};

State::State_t *ECUStates::Idle_State::run(void) {
    Log.i(ID, "Waiting for Button not to be pressed");
    while (!Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT_OFF)) {
    }

    Log.i(ID, "Waiting for Button or Charging Press");

    // Front teensy should already be blinking start light

    elapsedMillis waiting;

    while (true) {
        if (!Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT_OFF)) {
            Log.i(ID, "Button Pressed");
            // Front teensy will stop blinking start light
            return &ECUStates::Button_State;
        } else if (Pins::getCanPinValue(PINS_INTERNAL_CHARGE_SIGNAL)) {
            Log.i(ID, "Charging Pressed");
            // Front teensy will continue blinking start light in charge state
            return &ECUStates::Charging_State;
        } else if (FaultCheck()) {
            Log.w(ID, "Fault in idle state");
            break;
        }
    }
    return &ECUStates::FaultState;
}

State::State_t *ECUStates::Charging_State::run(void) {
    Pins::setPinValue(PINS_BACK_CHARGING_RELAY, HIGH);
    Log.i(ID, "Charging on");

    elapsedMillis voltLogNotify;

    while (Pins::getCanPinValue(PINS_INTERNAL_CHARGE_SIGNAL)) {
        // IMPROVE: Don't use fault to stop charging
        if (FaultCheck()) {
            Pins::setPinValue(PINS_BACK_CHARGING_RELAY, LOW);
            Log.e(ID, "Charging faulted, turning off");
            return &ECUStates::FaultState;
        }
    }

    Pins::setPinValue(PINS_BACK_CHARGING_RELAY, LOW);
    Log.i(ID, "Charging turning off");

    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::Button_State::run(void) {
    Log.i(ID, "Waiting for Button not to be pressed");
    while (!Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT_OFF)) {
    }
    Log.i(ID, "Playing sound");

    Pins::setPinValue(PINS_BACK_SOUND_DRIVER, PINS_ANALOG_HIGH);

    elapsedMillis soundTimer;

    while (soundTimer < 3000) {
        if (FaultCheck()) {
            Log.e(ID, "Failed to play sound");
            Pins::setPinValue(PINS_BACK_SOUND_DRIVER, LOW);
            return &ECUStates::FaultState;
        }
    }

    Pins::setPinValue(PINS_BACK_SOUND_DRIVER, LOW);
    Log.i(ID, "Playing sound finished");

    return &ECUStates::Driving_Mode_State;
}

void ECUStates::Driving_Mode_State::carCooling(bool enable) { // NOTE: Cooling values are all static
    Pins::setPinValue(PINS_BACK_PUMP_DAC, enable * PINS_ANALOG_MAX);
    int fanSet = enable * PINS_ANALOG_MAX / 2;
    Pins::setPinValue(PINS_BACK_FAN1_PWM, fanSet);
    Pins::setPinValue(PINS_BACK_FAN2_PWM, fanSet);
    Pins::setPinValue(PINS_BACK_FAN3_PWM, fanSet);
    Pins::setPinValue(PINS_BACK_FAN4_PWM, fanSet);
    Pins::setPinValue(PINS_BACK_FANS_ONOFF, enable);
}

State::State_t *ECUStates::Driving_Mode_State::DrivingModeFault(void) {
    Log.i(ID, "Fault happened in driving state");
    carCooling(false);
    Log.i(ID, "Starting MC heartbeat");
    MC::enableMotorBeating(true);
    return &ECUStates::FaultState;
}

// NOTE: MCs weak faults also cause a true fault here
State::State_t *ECUStates::Driving_Mode_State::run(void) {
    Log.i(ID, "Driving mode on");
    Log.i(ID, "Cooling on");
    carCooling(true);

    elapsedMillis controlDelay;
    size_t counter = 0;

    Log.i(ID, "Loading Buffers");
    MC0_VOLT_Buffer.init();
    MC1_VOLT_Buffer.init();

    Log.i(ID, "Stopping MC heartbeat");
    MC::enableMotorBeating(false);

    Log.d(ID, "Sending Fault reset to MCs complete");
    MC::clearFaults(); // Clear fault if any

    Log.d(ID, "Entering drive loop");
    while (true) {
        if (controlDelay > 5) { // NOTE: Each data frame is 89 bits long thus at 250kbps the MC buses can handle a maximum of 2808 messages per second
            controlDelay = 0;

            static bool runReverse = false;
            if (counter % 5 == 0 && Pins::getCanPinValue(PINS_INTERNAL_REVERSE) != runReverse) {
                runReverse = Pins::getCanPinValue(PINS_INTERNAL_REVERSE);
                MC::setDirection(!runReverse);
            }

            if (Fault::softFault() || Fault::hardFault()) { // FIXME: are motor controller faults actually being picked up?
#if TESTING != BACK_ECU
                return DrivingModeFault();
#endif
            }

            if (((MC0_VOLT_Buffer.getShort(0) / 10) + (MC1_VOLT_Buffer.getShort(0) / 10)) / 2 < 90) { // 'HVD Fault'
#if TESTING != BACK_ECU
                Log.e(ID, "'HVD Fault' MC voltage < 90");
                return DrivingModeFault();
#endif
            }

            int breakVal = Pins::getCanPinValue(PINS_FRONT_BRAKE);
            int steerVal = Pins::getCanPinValue(PINS_FRONT_STEER);

            Pins::setPinValue(PINS_BACK_BRAKE_LIGHT, PINS_ANALOG_HIGH * ((float)breakVal / PINS_ANALOG_MAX > 0.04f));

            int pedal0 = Pins::getCanPinValue(PINS_FRONT_PEDAL0);
            int pedal1 = Pins::getCanPinValue(PINS_FRONT_PEDAL1);

            int pAVG = (pedal0 + pedal1) / 2;

            // NOTE: pedal has a threshold value
            if (pAVG >= 100 && (float)abs(pedal1 - pedal0) / PINS_ANALOG_HIGH > 0.1f) {
                Log.e(ID, "Pedal value offset > 10%");
                Log.i(ID, "Pedal 0", pedal0);
                Log.i(ID, "Pedal 1", pedal1);
#if TESTING != BACK_ECU
                return DrivingModeFault();
#endif
            }

            MC::setTorque(pAVG, breakVal, steerVal);

            if (++counter > 128) {
                counter = 0;
                Log.i(ID, "Pedal 0", pedal0);
                Log.i(ID, "Pedal 1", pedal1);
                Log.d(ID, "Pedal AVG", pAVG);
                Log.d(ID, "Pedal value", MC::getLastPedalValue());
                Log.i(ID, "Brake value:", breakVal);
                Log.i(ID, "Steer value:", steerVal);
                Log.i(ID, "Aero servo position:", Aero::getServoValue());
                Log.i(ID, "Last MC0 Torque Value:", MC::getLastTorqueValue(true));
                Log.i(ID, "Last MC1 Torque Value:", MC::getLastTorqueValue(false));
                if (Fault::softFault()) {
                    Fault::logFault();
                }
            }

            Aero::run(breakVal, steerVal);
        }

        if (!Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT_OFF)) {
            Log.w(ID, "Going back to Idle state");
            break;
        }
    }

    Log.i(ID, "Starting MC heartbeat");
    MC::enableMotorBeating(true);

    carCooling(false);

    Log.i(ID, "Driving mode off");
    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::FaultState::run(void) {
    Pins::setInternalValue(PINS_INTERNAL_GEN_FAULT, 1);
    Canbus::enableInterrupts(false);

    Log.w(ID, "Opening Air1, Air2 and Precharge Relay");
    Pins::setPinValue(PINS_BACK_AIR1, LOW);
    Pins::setPinValue(PINS_BACK_AIR2, LOW);
    Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, LOW);

    Log.w(ID, "Resetting pins");
    Pins::resetPhysicalPins();

    if (getLastState() == &ECUStates::PreCharge_State) {
        Log.f(ID, "Precharge fault");
        while (true) {
            Pins::setInternalValue(PINS_INTERNAL_GEN_FAULT, 1);
            Fault::logFault();
            delay(1000);
        }
    } else {
        Log.e(ID, "FAULT STATE");
        Fault::logFault();
        delay(10000);
    }
    Pins::setInternalValue(PINS_INTERNAL_GEN_FAULT, 0);
    return &ECUStates::Initialize_State;
}

State::State_t *ECUStates::Logger_t::run(void) {

    static elapsedMillis timeElapsed;

    if (timeElapsed >= 2000) {
        timeElapsed = timeElapsed - 2000;
        Log(ID, "A7 Pin Value:", Pins::getPinValue(0));
        Log("FAKE ID", "A7 Pin Value:");
        Log(ID, "whaAAAT?");
        Log(ID, "", 0xDEADBEEF);
        Log(ID, "Notify code: ", getNotify());
    }

    return &ECUStates::Bounce;
};

State::State_t *ECUStates::Bounce_t::run(void) {
    delay(250);
    Log.i(ID, "Bounce!");
    notify(random(100));
    delay(250);
    return getLastState();
}