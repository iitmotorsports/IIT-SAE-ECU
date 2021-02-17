#include "ECUStates.hpp"
#include "ECUGlobalConfig.h"
#include "Faults.h"
#include "Log.h"

static bool FaultCheck() { // NOTE: Will only return true if hardfault occurs
    if (Fault::softFault())
        Fault::logFault();
    if (Fault::hardFault())
        return true;
    return false;
}

State::State_t *ECUStates::Initialize::run(void) {
    Log.i(ID, "Teensy 3.6 SAE BACK ECU Initalizing");
    Canbus::setup();    // allocate and organize addresses
    Pins::initialize(); // setup predefined pins
    Fault::setup();     // load all buffers

    if (FaultCheck()) {
        return &ECUStates::FaultState;
    }

    Log.i(ID, "Waiting for TSV Signal");
    while (!Pins::getPinValue(PINS_BACK_SHUTDOWN_SIGNAL)) {
    }
    Log.i(ID, "TSV On");

    Log.d(ID, "Finshed Setup");
    return &ECUStates::PreCharge_State;
};

State::State_t *ECUStates::PreCharge_State::PreCharFault(void) {
    Log.w(ID, "Opening Air1, Air2 and Precharge Relay");
    Pins::setPinValue(PINS_BACK_AIR1, LOW);
    Pins::setPinValue(PINS_BACK_AIR2, LOW);
    Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, LOW);
    return &ECUStates::FaultState;
}

// TODO: generalize buffer interpretation

bool ECUStates::PreCharge_State::voltageCheck() {
    Canbus::setSemaphore(ADD_BMS_VOLT);
    int16_t BMSVolt = *((int16_t *)(BMS_VOLT_Buffer + 2)); // Byte 2-3: Pack Instant Voltage
    Canbus::setSemaphore(ADD_MC0_VOLT);
    int16_t MC0Volt = *((int16_t *)(MC0_VOLT_Buffer)) / 10; // Bytes 0-1: DC BUS MC Voltage
    Canbus::setSemaphore(ADD_MC1_VOLT);
    int16_t MC1Volt = *((int16_t *)(MC1_VOLT_Buffer)) / 10; // Bytes 0-1: DC BUS MC Voltage
    Canbus::clearSemaphore();
    return 0.9 * BMSVolt <= (MC0Volt + MC1Volt) / 2;
}

void ECUStates::PreCharge_State::getBuffers() {
    BMS_VOLT_Buffer = Canbus::getBuffer(ADD_BMS_VOLT);
    MC0_VOLT_Buffer = Canbus::getBuffer(ADD_MC0_VOLT);
    MC1_VOLT_Buffer = Canbus::getBuffer(ADD_MC1_VOLT);
};

State::State_t *ECUStates::PreCharge_State::run(void) { // NOTE: Low = Closed, High = Open
    Log.i(ID, "Loading Buffers");
    getBuffers();
    Log.i(ID, "Precharge running");

    if (FaultCheck()) {
        Log.e(ID, "Precharge Faulted before charge");
        return PreCharFault();
    }

    // NOTE: Assuming Air2 is correct
    Log.w(ID, "Closing Air2 and Precharge Relay and opening Air1");
    Pins::setPinValue(PINS_BACK_AIR2, HIGH);
    Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, HIGH);
    Pins::setPinValue(PINS_BACK_AIR1, LOW);

    if (FaultCheck()) {
        Log.e(ID, "Precharge Faulted after charge");
        return PreCharFault();
    }

    elapsedMillis timeElapsed;

    while (timeElapsed < 5000) {
        if (voltageCheck()) {
            Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, LOW);
            Log.i(ID, "Precharge Finished, closing Air1");
            Pins::setPinValue(PINS_BACK_AIR1, LOW);
            return &ECUStates::Idle_State;
        }
    }

    Log.e(ID, "Precharge timed out");
    return PreCharFault();
};

State::State_t *ECUStates::Idle_State::run(void) {
    Log.i(ID, "Waiting for Button or Charging Press");
    Pins::setInternalValue(PINS_INTERNAL_IDLE_STATE, HIGH);

    elapsedMillis waiting;

    while (true) {
        if (Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT)) {
            Log.i(ID, "Button Pressed");
            Pins::setInternalValue(PINS_INTERNAL_IDLE_STATE, LOW);
            return &ECUStates::Button_State;
        } else if (Pins::getCanPinValue(PINS_INTERNAL_CHARGE_SIGNAL)) {
            Log.i(ID, "Charging Pressed");
            Pins::setInternalValue(PINS_INTERNAL_IDLE_STATE, LOW);
            return &ECUStates::Charging_State;
        } else if (FaultCheck()) {
            break;
        }
        if (waiting >= 1000) { // Notify every secondish
            waiting = 0;
            Log.d(ID, "Waiting for button press");
        }
    }
    Pins::setInternalValue(PINS_INTERNAL_IDLE_STATE, LOW);
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

    // motor controller enable bit off
    Pins::setPinValue(PINS_BACK_SOUND_DRIVER, HIGH);

    elapsedMillis soundTimer;

    while (soundTimer <= 2000) {
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
    Log.d(ID, "Torque percent: ", percentTorque);
    // Calculations value = (high_byte x 256) + low_byte
    uint8_t low_byte = percentTorque % 256;
    uint8_t high_byte = percentTorque / 256;
    Canbus::sendData(MC_ADD, low_byte, high_byte, 0, 0, direction, enableBit); // TODO: check that low and high are in the right place
}

void ECUStates::Driving_Mode_State::torqueVector(int torques[2], float pedalVal) {
    torques[0] = pedalVal; // TODO: Add Torque vectoring algorithms
    torques[1] = pedalVal;
}

void ECUStates::Driving_Mode_State::carCooling(float temp) { // TODO: map temp to voltages
    Pins::setPinValue(PINS_BACK_PUMP_DAC, (int)map(temp, 0, 100, 4095.0f * 2.0f / 5, 4095.0f * 2.5f / 5));
    int fanSet = (int)map(temp, 0, 100, 0, 4095);
    Pins::setPinValue(PINS_BACK_FAN1_PWM, fanSet);
    Pins::setPinValue(PINS_BACK_FAN2_PWM, fanSet);
    Pins::setPinValue(PINS_BACK_FAN3_PWM, fanSet);
    Pins::setPinValue(PINS_BACK_FAN4_PWM, fanSet);
}

State::State_t *ECUStates::Driving_Mode_State::run(void) {
    Log.i(ID, "Driving mode on");

    Pins::setInternalValue(PINS_INTERNAL_START, 1);

    while (true) {
        Canbus::update();
        // int BMSTemp = 30;
        // carCooling((float)BMSTemp); // TODO: what temp are we using for cooling?

        for (size_t i = 0; i < 4; i++) {               // IMPROVE: Send only once? Check MC heartbeat catches it
            Canbus::sendData(ADD_MC0_CLEAR, 20, 0, 1); // NOTE: We are assuming MCs are little endian
            Canbus::sendData(ADD_MC1_CLEAR, 20, 0, 1);
            delay(10);
        }

        Log.d(ID, "Sending Fault reset to MCs complete");

        int pedal0 = Pins::getCanPinValue(PINS_FRONT_PEDAL0);
        int pedal1 = Pins::getCanPinValue(PINS_FRONT_PEDAL1);
        if (abs(pedal1 - pedal0) > (float)pedal0 * 5 / 100) {
            Log.e(ID, "Pedal value offset > 5%");
            return &ECUStates::FaultState;
        }

        if (Pins::getCanPinValue(PINS_FRONT_BRAKE) / 4095 > 4) { // NOTE: analog res is at 12 bits which means 4095 is max value
            Pins::setPinValue(PINS_BACK_BRAKE_LIGHT, 4095);
        } else {
            Pins::setPinValue(PINS_BACK_BRAKE_LIGHT, 0);
        }

        int MotorTorques[2] = {0};
        torqueVector(MotorTorques, (float)(pedal0 + pedal1) / 2);

        sendMCCommand(ADD_MC0_CTRL, MotorTorques[0], 0, 0); // MC 1
        sendMCCommand(ADD_MC1_CTRL, MotorTorques[1], 1, 0); // MC 2

        if (Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT)) {
            Log.w(ID, "Going back to Idle state");
            break;
        }

        if (FaultCheck()) {
            return &ECUStates::FaultState;
        }
    }

    Log.i(ID, "Driving mode off");
    Pins::setInternalValue(PINS_INTERNAL_START, 0);
    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::FaultState::run(void) {
    Canbus::enableInterrupts(false);

    Log.w(ID, "Opening Air1, Air2 and Precharge Relay");
    Pins::setPinValue(PINS_BACK_AIR1, LOW);
    Pins::setPinValue(PINS_BACK_AIR2, LOW);
    Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, LOW);

    Log.w(ID, "Resetting pins");
    Pins::resetPhysicalPins();

    Log.d(ID, "Setting fault canpins");
    Pins::setInternalValue(PINS_INTERNAL_START, 0);
    Pins::setInternalValue(PINS_INTERNAL_BMS_FAULT, 1);
    Pins::setInternalValue(PINS_INTERNAL_IMD_FAULT, 1);

    delay(1000);
    Log.e(ID, "Stopping canpins");
    Pins::stop();

    while (true) {
        Log.f(ID, "FAULT STATE");
        Fault::logFault();
        delay(1000);
    }
    return &ECUStates::FaultState;
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
    State::notify(random(100));
    delay(250);
    return State::getLastState();
}