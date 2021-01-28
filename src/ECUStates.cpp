#include "ECUStates.hpp"
#include "Faults.h"
#include "Log.h"

State::State_t *ECUStates::Initialize::run(void) {
    Log.i(ID, "Teensy 3.6 SAE ECU Initalizing");
    pinMode(6, OUTPUT);
    pinMode(13, OUTPUT);
    digitalWrite(6, LOW); /* optional CAN transceiver enable pin */
    Canbus::setup();      // Interrupts not enabled
    Pins::initialize();   // setup predefined pins

    // TODO: TSV

    Log.d(ID, "Finshed");
    return &ECUStates::PreCharge_State;
};

State::State_t *ECUStates::PreCharge_State::PreCharFault(void) {
    Log.w(ID, "Closing Air1 and Precharge Relay");
    Pins::setPinValue(Air1_Pin, LOW);
    Pins::setPinValue(Precharge_Relay_Pin, LOW);
    return &ECUStates::FaultState;
}

bool ECUStates::PreCharge_State::voltageCheck() {     // TODO: canbus semaphore
    uint BMSVolt = *(uint *)(BMS_Voltage_Buffer + 1); // TODO: get BMS volt from buffer
    uint MCVolt = *(uint *)(MC0_Voltage_Buffer + 1);  // TODO: get MC volt from buffer
    return 0.9 * BMSVolt <= MCVolt;
}

State::State_t *ECUStates::PreCharge_State::run(void) { // FIXME: set pins to LOW or HIGH?
    Log.i(ID, "Precharge running");

    if (Fault::hardFault()) {
        Log.e(ID, "Precharge Faulted before charge");
        return PreCharFault();
    }

    Log.w(ID, "Opening Air1 and Precharge Relay");
    Pins::setPinValue(Air1_Pin, HIGH);
    Pins::setPinValue(Precharge_Relay_Pin, HIGH);

    if (Fault::hardFault()) {
        Log.e(ID, "Precharge Faulted after charge");
        return PreCharFault();
    }

    elapsedMillis timeElapsed;

    while (timeElapsed < 5000) {
        if (voltageCheck()) {
            Pins::setPinValue(Precharge_Relay_Pin, LOW);
            Log.i(ID, "Precharge Finished");
            Pins::setPinValue(Air2_Pin, HIGH);
            return &ECUStates::Idle_State;
        }
    }

    Log.e(ID, "Precharge timed out");
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
        } else if (Fault::hardFault()) {
            break;
        }

        Log.d(ID, "Waiting");
    }

    return &ECUStates::FaultState;
}

State::State_t *ECUStates::Charging_State::run(void) {
    Pins::setPinValue(Charging_Relay_Pin, HIGH);
    Log.i(ID, "Charging on");

    while (Pins::getPinValue(Charging_Signal_Pin)) {
        // IMPROVE: Don't use fault to stop charging
        if (Fault::hardFault()) {
            Pins::setPinValue(Charging_Relay_Pin, LOW);
            Log.e(ID, "Charging faulted, turning off");
            return &ECUStates::FaultState;
        }
        Log.i(ID, "Voltage", Pins::getPinValue(Charging_Voltage_Pin)); // delay
    }

    Pins::setPinValue(Charging_Relay_Pin, LOW);
    Log.i(ID, "Charging turning off");

    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::Button_State::run(void) {
    Log.i(ID, "Playing sound");

    // motor controller enable bit off
    Pins::setPinValue(Sound_Driver_Pin, HIGH);

    elapsedMillis soundTimer;

    while (soundTimer <= 2000) {
        if (false) { // TODO: Check for fault
            Log.e(ID, "Failed to play sound");
            Pins::setPinValue(Sound_Driver_Pin, LOW);
            return &ECUStates::FaultState;
        }
    }

    Pins::setPinValue(Sound_Driver_Pin, LOW);
    Log.i(ID, "Playing sound finished");

    return &ECUStates::Driving_Mode_State;
}

// static uint32_t MOTOR_OFFSET = 0xe0;         // offset for motor ids // is this actually just for the MCs?
// static uint32_t MOTOR_STATIC_OFFSET = 0x0A0; // IMPROVE: auto set this global offset to addresses

// void motorWriteSpeed(TTMsg msg, byte offset, bool direction, int speed) { // speed is value 0 - 860
//     int percent_speed = constrain(map(speed, 0, 1024, 0, 400), 0, 400);   // seprate func for negative vals (regen)
//     // Serial.println(percent_speed);
//     //Calculations value = (high_byte x 256) + low_byte
//     byte low_byte = percent_speed % 256;
//     byte high_byte = percent_speed / 256;
//     msg.id = SPEEDWRITE_ADD + offset - MOTOR_STATIC_OFFSET;
//     // Serial.println(msg.id);
//     msg.ext = 0;
//     msg.len = 8;
//     msg.buf[0] = low_byte; // NM
//     msg.buf[1] = high_byte;
//     msg.buf[2] = 0; // Speed
//     msg.buf[3] = 0;
//     msg.buf[4] = direction;           // Direction
//     msg.buf[5] = START_BUTTON_PUSHED; // Inverter enable byte
//     msg.buf[6] = 0;                   // Last two are the maximum torque values || if 0 then defualt values are set
//     msg.buf[7] = 0;
//     writeTTMsg(msg);
// }

void ECUStates::Driving_Mode_State::sendMCCommand(uint32_t MC_ADD, int torque, bool direction, bool enableBit) {
}

void ECUStates::Driving_Mode_State::torqueVector(int torques[2]) {
    int pedal = 0; // Read Can message PEDALVALUE // NOTE: Receive from front teensy
                   // TODO: Add Torque vectoring algorithms
    torques[0] = pedal;
    torques[1] = pedal;
}

uint32_t ECUStates::Driving_Mode_State::BMSSOC() { // TODO: canbus semaphore
    return *(uint *)(BMS_SOC_Buffer + 2);
}

uint32_t ECUStates::Driving_Mode_State::powerValue() { // TODO: canbus semaphore
    uint MC0_PWR = *(uint *)(MC0_PWR_Buffer + 4);
    uint MC1_PWR = *(uint *)(MC1_PWR_Buffer + 4);
    return MC0_PWR + MC1_PWR;
}

State::State_t *ECUStates::Driving_Mode_State::run(void) {
    Log.i(ID, "Driving mode on");

    while (true) {
        Canbus::update();
        // TODO: Send fault reset to MCs
        Log.d(ID, "Sending Fault reset to MCs complete");

        for (size_t i = 0; i < 4; i++) { // IMPROVE: Send only once? Check MC heartbeat catches it
            sendMCCommand(0x0, 0, 0, 0); // MC 1
            sendMCCommand(0x0, 0, 1, 0); // MC 2
            delay(10);
        }

        int MotorTorques[2];
        torqueVector(MotorTorques);

        sendMCCommand(0x0, MotorTorques[0], 0, 0); // MC 1
        sendMCCommand(0x0, MotorTorques[1], 1, 0); // MC 2

        /* Front Teensy Exclusive */
        // GAUGE
        // receive rpm of MCs, interpret, then send to from teensy for logging
        int MC_Rpm_Val_1 = 0;            // Canbus message from MCs
        int MC_Rpm_Val_2 = 0;            // Canbus message from MCs
        int MC_Spd_Val_1 = MC_Rpm_Val_1; // TODO: convert RPM -> SPD
        int MC_Spd_Val_2 = MC_Rpm_Val_2;
        int speed = (MC_Spd_Val_1 + MC_Spd_Val_2) / 2; // Send to front teensy
        Log.i(ID, "Current Motor Speed:", speed);
        Log.i(ID, "Current Power Value:", powerValue());   // Canbus message from MCs
        Log.i(ID, "BMS State Of Charge Value:", BMSSOC()); // Canbus message
        /* End Front Teensy Exclusive */

        Log.w(ID, "Going back to Idle state");
        // push button is same as idlestate

        // Pins::update(); // TODO: uncomment once analog caching is done
        if (Fault::hardFault()) {
        }
    }

    Log.i(ID, "Driving mode off");
    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::FaultState::run(void) { // IMPROVE: Should fault state do anything else?
    Canbus::enableInterrupts(false);
    Log.f(ID, "FAULT STATE");
    Fault::logFault();
    delay(250);
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