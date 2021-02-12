#include "ECUStates.hpp"
#include "ECUGlobalConfig.h"
#include "Faults.h"
#include "Log.h"

// TODO: Separate teensy file for front teensy

static bool FaultCheck() { // NOTE: Will only return true if hardfault occurs
    if (Fault::softFault())
        Fault::logFault();
    if (Fault::hardFault())
        return true;
    return false;
}

State::State_t *ECUStates::Initialize::run(void) {
#if CONF_ECU_POSITION == BACK_ECU
    Log.i(ID, "Teensy 3.6 SAE BACK ECU Initalizing");
#else
    Log.i(ID, "Teensy 3.6 SAE FRONT ECU Initalizing");
#endif
    Canbus::setup();    // allocate and organize addresses
    Pins::initialize(); // setup predefined pins
    Fault::setup();     // load all buffers
#if CONF_ECU_POSITION == FRONT_ECU
    Logging::enableCanbusRelay(); // Allow logging though canbus
#endif

    Pins::setPinValue(PINS_BOTH_OPT_CAN_TRAN, LOW); // optional CAN transceiver enable pin

    // FIXME: Temporary
    Pins::setPinValue(PINS_BACK_CHARGING_RELAY, HIGH);

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

    while (true) {
        if (Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT)) {
            Log.i(ID, "Button Pressed");
            return &ECUStates::Button_State;
        } else if (Pins::getPinValue(PINS_BACK_CHARGING_INPUT)) { // TODO: replace with tablet input
            Log.i(ID, "Charging Pressed");
            return &ECUStates::Charging_State;
        } else if (FaultCheck()) {
            break;
        }
        Log.d(ID, "Waiting");
    }
    return &ECUStates::FaultState;
}

State::State_t *ECUStates::Charging_State::run(void) {
    Pins::setPinValue(PINS_BACK_CHARGING_RELAY, HIGH);
    Log.i(ID, "Charging on");

    elapsedMillis voltLogNotify;

    while (Pins::getPinValue(PINS_BACK_CHARGING_SIGNAL)) {
        // IMPROVE: Don't use fault to stop charging
        if (FaultCheck()) {
            Pins::setPinValue(PINS_BACK_CHARGING_RELAY, LOW);
            Log.e(ID, "Charging faulted, turning off");
            return &ECUStates::FaultState;
        }
        if (voltLogNotify >= 1000) { // Notify every secondish
            voltLogNotify = 0;
            Log.i(ID, "Voltage", Pins::getPinValue(PINS_BACK_CHARGING_VOLTAGE));
        }
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

uint32_t ECUStates::Driving_Mode_State::BMSSOC() {
    Canbus::setSemaphore(ADD_BMS_SOC);
    return *(uint8_t *)(BMS_SOC_Buffer + 4); // Byte 4: BMS State of charge buffer
    Canbus::clearSemaphore();
}

uint32_t ECUStates::Driving_Mode_State::powerValue() { // IMPROVE: get power value using three phase values, or find a power value address
    Canbus::setSemaphore(ADD_MC0_VOLT);
    int16_t MC0_VOLT = *((int16_t *)(MC0_VOLT_Buffer)) / 10; // Bytes 0-1: DC BUS MC Voltage
    Canbus::setSemaphore(ADD_MC0_CURR);
    int16_t MC0_CURR = *((int16_t *)(MC0_CURR_Buffer + 6)) / 10; // Bytes 6-7: DC BUS MC Current
    int MC0_PWR = MC0_VOLT * MC0_CURR;
    Canbus::setSemaphore(ADD_MC1_VOLT);
    int16_t MC1_VOLT = *((int16_t *)(MC1_VOLT_Buffer)) / 10; // Bytes 0-1: DC BUS MC Voltage
    Canbus::setSemaphore(ADD_MC1_CURR);
    int16_t MC1_CURR = *((int16_t *)(MC1_CURR_Buffer + 6)) / 10; // Bytes 6-7:
    int MC1_PWR = MC1_VOLT * MC1_CURR;
    Canbus::clearSemaphore();
    return (MC0_PWR + MC1_PWR) / 1000; // Sending kilowatts
}

void ECUStates::Driving_Mode_State::getBuffers() {
    MC0_RPM_Buffer = Canbus::getBuffer(ADD_MC0_RPM);
    MC1_RPM_Buffer = Canbus::getBuffer(ADD_MC1_RPM);
    MC0_VOLT_Buffer = Canbus::getBuffer(ADD_MC0_VOLT);
    MC1_VOLT_Buffer = Canbus::getBuffer(ADD_MC1_VOLT);
    MC0_CURR_Buffer = Canbus::getBuffer(ADD_MC0_CURR);
    MC1_CURR_Buffer = Canbus::getBuffer(ADD_MC1_CURR);
    BMS_SOC_Buffer = Canbus::getBuffer(ADD_BMS_SOC);
}

State::State_t *ECUStates::Driving_Mode_State::run(void) {
    Log.i(ID, "Loading Buffers");
    getBuffers();
    Log.i(ID, "Driving mode on");

    // TODO: turn on LED on front teensy

#if CONF_ECU_POSITION == FRONT_ECU
    elapsedMillis timeElapsed;
#endif

    while (true) {
        Canbus::update();
#if CONF_ECU_POSITION == FRONT_ECU
        if (timeElapsed >= 5) { // Update Tablet every 5ms
            timeElapsed = 0;
            // receive rpm of MCs, interpret, then send to from teensy for logging
            Canbus::setSemaphore(ADD_MC0_RPM);
            int16_t MC_Rpm_Val_0 = *(int16_t *)(MC0_RPM_Buffer + 2); // Bytes 2-3 : Angular Velocity
            Canbus::setSemaphore(ADD_MC1_RPM);
            int16_t MC_Rpm_Val_1 = *(int16_t *)(MC1_RPM_Buffer + 2); // Bytes 2-3 : Angular Velocity
            Canbus::clearSemaphore();
            float MC_Spd_Val_0 = wheelRadius * 2 * 3.1415926536 / 60 * MC_Rpm_Val_0;
            float MC_Spd_Val_1 = wheelRadius * 2 * 3.1415926536 / 60 * MC_Rpm_Val_1;
            float speed = (MC_Spd_Val_0 + MC_Spd_Val_1) / 2;
            Log.i(ID, "Current Motor Speed:", speed);
            Log.i(ID, "Current Power Value:", powerValue());   // Canbus message from MCs
            Log.i(ID, "BMS State Of Charge Value:", BMSSOC()); // Canbus message
        }
#else
        // TODO: figure out DAC, output 2-2.5V to pump pin and fans using a cooling function

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

        if (Pins::getCanPinValue(PINS_FRONT_BRAKE) / ANALOGMAX > 4) { // TODO: divide by whatever ADC max value is
            Pins::setPinValue(PINS_BACK_BRAKE_LIGHT, HIGH);
        } else {
            Pins::setPinValue(PINS_BACK_BRAKE_LIGHT, LOW);
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
#endif
    }

    Log.i(ID, "Driving mode off");
    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::FaultState::run(void) { // IMPROVE: Should fault state do anything else?
    Canbus::enableInterrupts(false);

    Log.w(ID, "Opening Air1, Air2 and Precharge Relay");
    Pins::setPinValue(PINS_BACK_AIR1, LOW);
    Pins::setPinValue(PINS_BACK_AIR2, LOW);
    Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, LOW);

    // TODO: Re-Initialize pins to starting state

    // TODO: Turn on appropriate Fault LEDs on front teensy, boolean receive from back teensy

    while (true) {
        Log.f(ID, "FAULT STATE");
        Fault::logFault();
        delay(250);
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