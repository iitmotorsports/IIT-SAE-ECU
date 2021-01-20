#include "ECUStates.hpp"
#include "Log.h"

State::State_t *ECUStates::Initialize::run(void) {
    Log.i(ID, "Teensy 3.6 SAE ECU Initalizing");
    pinMode(6, OUTPUT);
    pinMode(13, OUTPUT);
    digitalWrite(6, LOW); /* optional CAN transceiver enable pin */
    Canbus::setup();      // Interrupts not enabled
    Pins::initialize();   // setup predefined pins

    Log.d(ID, "Finshed");
    return &ECUStates::PreCharge_State;
};

State::State_t *ECUStates::PreCharge_State::PreCharFault(void) {
    Log.e(ID, "Precharge Fault");
    // Close Air1_Pin and Precharge_Relay_Pin for any fault
    Pins::setPinValue(Air1_Pin, LOW);
    Pins::setPinValue(Precharge_Relay_Pin, LOW);
    return &ECUStates::Fault;
}

bool ECUStates::PreCharge_State::checkPreFault(void) { // TODO: Check for what fault?
    return false;
}

State::State_t *ECUStates::PreCharge_State::run(void) {
    Log.i(ID, "Precharge running");

    if (checkPreFault()) {
        return PreCharFault();
    }

    // Open Air1_Pin and Precharge_Relay_Pin
    Pins::setPinValue(Air1_Pin, HIGH);
    Pins::setPinValue(Precharge_Relay_Pin, HIGH);

    if (checkPreFault()) {
        return PreCharFault();
    }

    elapsedMillis timeElapsed;

    while (timeElapsed < 5000) {
        if (0.9 * Pins::getPinValue(BMS_Voltage_Pin) <= Pins::getPinValue(MC_Voltage_Pin)) {
            Pins::setPinValue(Precharge_Relay_Pin, LOW);
            Log.i(ID, "Precharge Finished");
            Pins::setPinValue(Air2_Pin, HIGH);
            return &ECUStates::Idle_State;
        }
    }

    return PreCharFault();
};

State::State_t *ECUStates::Idle_State::run(void) {
    Log.i(ID, "Waiting for Button or Charging Press");

    while (true) {
        if (Pins::getPinValue(Button_Input_Pin)) {
            Log.i(ID, "Button Pressed");
            return &ECUStates::Button_State;
        } else if (Pins::getPinValue(Charging_Input_Pin)) {
            Log.i(ID, "Charging Pressed");
            return &ECUStates::Charging_State;
        }
        Log.d(ID, "Waiting");
    }

    return &ECUStates::Fault;
}

bool ECUStates::Charging_State::checkChargingFault(void) { // TODO: what fault?
    return false;
}

State::State_t *ECUStates::Charging_State::run(void) {
    Pins::setPinValue(Charging_Relay_Pin, HIGH);
    Log.i(ID, "Charging on");

    while (Pins::getPinValue(Charging_Signal_Pin)) {
        if (checkChargingFault()) {
            Pins::setPinValue(Charging_Relay_Pin, LOW);
            Log.e(ID, "Charging faulted, turning off");
            return &ECUStates::Fault;
        }
        Log.i(ID, "Voltage", Pins::getPinValue(Charging_Voltage_Pin));
    }

    Pins::setPinValue(Charging_Relay_Pin, LOW);
    Log.i(ID, "Charging turning off");

    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::Button_State::run(void) {
    Log.i(ID, "Playing sound");
    // Turn Ready to drive signal on and initialize Sound_timer ?

    elapsedMillis soundTimer;

    while (soundTimer <= 2000) {
        if (false) { // TODO: Check for fault
            Log.e(ID, "Failed to play sound");
            return &ECUStates::Fault;
        }
    }
    Log.i(ID, "Playing sound finished");

    return &ECUStates::Driving_Mode_State;
}

State::State_t *ECUStates::Driving_Mode_State::run(void) {
    Log.i(ID, "Driving mode on");
    Canbus::enableInterrupts(true);

    while (true) {
        Canbus::update();
        // Pins::update(); // TODO: uncomment once analog caching is done
    }

    Canbus::enableInterrupts(false);
    Log.i(ID, "Driving mode off");
    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::Fault::run(void) { // TODO: Implement Fault state
    Canbus::enableInterrupts(false);
    Log.f(ID, "FAULT STATE");
    return &ECUStates::Fault;
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