#include "ECUStates.hpp"
#include "AeroServo.h"
#include "ECUGlobalConfig.h"
#include "Faults.h"
#include "Heartbeat.h"
#include "Log.h"
#include "Mirror.h"

static bool FaultCheck() { // NOTE: Will only return true if hardfault occurs
    if (Fault::softFault())
        Fault::logFault();
    if (Fault::hardFault())
        return true;
    return false;
}

State::State_t *ECUStates::Initialize_State::run(void) {
    Log.i(ID, "Teensy 3.6 SAE BACK ECU Initalizing");
#ifdef CONF_ECU_DEBUG
    delay(2000); // prevent test faults to continuosly loop
#endif
    Canbus::setup();    // allocate and organize addresses
    Pins::initialize(); // setup predefined pins
    Fault::setup();     // load all buffers
    Aero::setup();
    Heartbeat::beginBeating();
#ifdef CONF_ECU_DEBUG
    Mirror::setup();
#endif

    // Front teensy should know when to blink start light

    if (FaultCheck()) {
        return &ECUStates::FaultState;
    }

    // TSV
    Log.i(ID, "Waiting for shutdown signal");
    while (!Pins::getPinValue(PINS_BACK_SHUTDOWN_SIGNAL)) {
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
    int16_t BMSVolt = BMS_DATA_Buffer.getShort(2);      // Byte 2-3: Pack Instant Voltage
    int16_t MC0Volt = MC0_VOLT_Buffer.getShort(0) / 10; // Bytes 0-1: DC BUS MC Voltage
    int16_t MC1Volt = MC1_VOLT_Buffer.getShort(0) / 10; // Bytes 0-1: DC BUS MC Voltage
    return 0.9 * BMSVolt <= (MC0Volt + MC1Volt) / 2;
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

    while (timeElapsed <= 5000) {
        if (voltageCheck()) {
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
    while (Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT)) {
    }

    Log.i(ID, "Waiting for Button or Charging Press");

    // Front teensy should already be blinking start light

    elapsedMillis waiting;

    while (true) {
        if (Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT)) {
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
        // if (voltLogNotify >= 1000) { // Notify every secondish
        //     voltLogNotify = 0;
        //     Log.i(ID, "Voltage", Pins::getPinValue(PINS_BACK_CHARGING_VOLTAGE)); // TODO: what voltage are we sending
        // }
    }

    Pins::setPinValue(PINS_BACK_CHARGING_RELAY, LOW);
    Log.i(ID, "Charging turning off");

    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::Button_State::run(void) {
    Log.i(ID, "Playing sound");

    Pins::setPinValue(PINS_BACK_SOUND_DRIVER, PINS_ANALOG_HIGH);

    elapsedMillis soundTimer;

    while (soundTimer < 2000) {
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

void ECUStates::Driving_Mode_State::sendMCCommand(uint32_t MC_ADD, int torque, bool direction, bool enableBit) {
    int percentTorque = constrain(map(torque, 0, 1024, 0, 400), 0, 400); // separate func for negative vals (regen)
    // Log.d(ID, "Torque percent: ", percentTorque);
    // Calculations value = (high_byte x 256) + low_byte
    uint8_t low_byte = percentTorque % 256;
    uint8_t high_byte = percentTorque / 256;
    Canbus::sendData(MC_ADD, low_byte, high_byte, 0, 0, direction, 0b11000000 * enableBit); // TODO: check that low and high are in the right place
}

void ECUStates::Driving_Mode_State::torqueVector(int torques[2], int pedal0, int pedal1) {
    // TODO: Add Torque vectoring algorithms
    int pedalVal = (pedal0 + pedal1) / 2;
    torques[0] = pedalVal; // TODO: must be in percentage value?
    torques[1] = pedalVal;
}

void ECUStates::Driving_Mode_State::carCooling(float temp) { // TODO: map temp to voltages
    // Pins::setPinValue(PINS_BACK_PUMP_DAC, (int)map(temp, 0, 100, PINS_ANALOG_HIGH * 2.0f / 5, PINS_ANALOG_HIGH * 2.5f / 5));
    // int fanSet = (int)map(temp, 0, 100, 0, PINS_ANALOG_HIGH);
    // Rashed says to just set them to high, shouldn't be capable of over-cooling
    Pins::setPinValue(PINS_BACK_PUMP_DAC, (int)(PINS_ANALOG_HIGH * 2.5f / 5.0f));
    int fanSet = PINS_ANALOG_HIGH;
    Pins::setPinValue(PINS_BACK_FAN1_PWM, fanSet);
    Pins::setPinValue(PINS_BACK_FAN2_PWM, fanSet);
    Pins::setPinValue(PINS_BACK_FAN3_PWM, fanSet);
    Pins::setPinValue(PINS_BACK_FAN4_PWM, fanSet);
}

State::State_t *ECUStates::Driving_Mode_State::DrivingModeFault(void) {
    Log.i(ID, "Fault happened in driving state");
    clearMCs();
    return &ECUStates::FaultState;
}

void ECUStates::Driving_Mode_State::clearMCs() {
    sendMCCommand(ADD_MC0_CTRL, 0, 0, 0);
    sendMCCommand(ADD_MC1_CTRL, 0, 1, 0);
}

void ECUStates::Driving_Mode_State::clearFault(void) {
    Canbus::sendData(ADD_MC0_CLEAR, 20, 0, 1); // NOTE: based off documentation example, MCs are little endian
    Canbus::sendData(ADD_MC1_CLEAR, 20, 0, 1);
}

State::State_t *ECUStates::Driving_Mode_State::run(void) {
    Log.i(ID, "Driving mode on");

    carCooling(60); // TODO: what temp are we using for cooling?

    for (size_t i = 0; i < 4; i++) { // IMPROVE: Send only once? Check MC heartbeat fault is actually cleared
        clearFault();
        delay(10);
    }

    Log.d(ID, "Sending Fault reset to MCs complete");

    clearMCs();

    elapsedMillis controlDelay;

    Log.d(ID, "Entering drive loop");
    while (true) {
        if (FaultCheck()) {
            return DrivingModeFault();
        }

        int pedal0 = Pins::getCanPinValue(PINS_FRONT_PEDAL0);
        int pedal1 = Pins::getCanPinValue(PINS_FRONT_PEDAL1);
        if (abs(pedal1 - pedal0) > (pedal0 * 5) / 100) {
            Log.e(ID, "Pedal value offset > 5%");
            Log.i(ID, "", pedal0);
            Log.i(ID, "", pedal1);
            return DrivingModeFault();
        }

        static int breakVal = Pins::getCanPinValue(PINS_FRONT_BRAKE);
        Pins::setPinValue(PINS_BACK_BRAKE_LIGHT, PINS_ANALOG_HIGH * ((breakVal / PINS_ANALOG_MAX) > 4));

        if (controlDelay > 10) {
            controlDelay = 0;
            int MotorTorques[2] = {0};
            torqueVector(MotorTorques, pedal0, pedal1);
            sendMCCommand(ADD_MC0_CTRL, MotorTorques[0], 0, 1); // MC 1
            sendMCCommand(ADD_MC1_CTRL, MotorTorques[1], 1, 1); // MC 2
        }

        Aero::run(breakVal, Pins::getCanPinValue(PINS_FRONT_STEER));

        if (Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT)) {
            Log.w(ID, "Going back to Idle state");
            break;
        }
    }

    clearMCs();

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
            Pins::setInternalValue(PINS_INTERNAL_BMS_FAULT, Pins::getPinValue(PINS_BACK_BMS_FAULT));
            Pins::setInternalValue(PINS_INTERNAL_IMD_FAULT, Pins::getPinValue(PINS_BACK_IMD_FAULT));
            Fault::logFault();
            delay(1000);
        }
    } else {
        Log.e(ID, "FAULT STATE");
        Fault::logFault();
        delay(1000);
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