#include "Front.h"
#include "ECUGlobalConfig.h"
#include "Echo.h"
#include "Heartbeat.h"
#include "Mirror.h"
#include "MotorControl.h"
#include "SerialCommand.h"
#include "SerialVar.h"
#include "unordered_map"
#if ECU_TESTING == FRONT_ECU
#include "Test.h"
#endif

namespace Front {

static elapsedMillis timeElapsedHigh;
static elapsedMillis timeElapsedMidHigh;
static elapsedMillis timeElapsedMidLow;
static elapsedMillis timeElapsedLow;

SerialVar::SerialFloat TVAggression = SERIALVAR_TORQUE_VECTORING_AGGRESSION;

static void pushCanMessage() {
    uint32_t address;
    char buffer[8];
    size_t c = 0;
    c += Serial.readBytes((char *)&address, 4);
    c += Serial.readBytes(buffer, 8);
    if (c != 12) {
        Log.w(ID, "Did not read correct number of Bytes, not pushing message", 12);
        return;
    }
    Log.d(ID, "Pushing Message", address);
    Canbus::sendData(address, (uint8_t *)buffer);
}

static void toggleCanbusSniffer() {
    static bool enabled = false;
    Canbus::enableCanbusSniffer((enabled = !enabled));
}

static void sendEchoMessage() {
    uint32_t delay;
    uint32_t address;
    char buffer[8];
    size_t c = 0;
    c += Serial.readBytes((char *)&delay, 4);
    c += Serial.readBytes((char *)&address, 4);
    c += Serial.readBytes(buffer, 8);
    if (c != 20) {
        Log.w(ID, "Did not read correct number of Bytes, not pushing message", 20);
        return;
    }
    Log.d(ID, "Pushing Message", address);
    Echo::echo(delay, address, (uint8_t *)buffer);
}

static void toggleMotorDirection() {
    static bool reverse = false;
    reverse = !reverse;
    Pins::setInternalValue(PINS_INTERNAL_REVERSE, reverse);
}

void blinkStart() {
    Pins::setPinValue(PINS_FRONT_START_LIGHT, 1);
    delay(500);
    Pins::setPinValue(PINS_FRONT_START_LIGHT, 0);
    delay(500);
}

void run() {
    Log.i(ID, "Teensy 3.6 SAE FRONT ECU Initalizing");

    Log.i(ID, "Setting up Canbus");
    Canbus::setup(); // allocate and organize addresses
    Log.i(ID, "Initalizing Pins");
    Pins::initialize(); // setup predefined pins
#ifndef CONF_LOGGING_ASCII_DEBUG
    Log.i(ID, "Enabling Logging relay");
    Logging::enableCanbusRelay(); // Allow logging though canbus
#endif
    loadStateMap();

    Log.i(ID, "Setting commands");
    Cmd::setCommand(COMMAND_ENABLE_CHARGING, setChargeSignal);
    Cmd::setCommand(COMMAND_SEND_CANBUS_MESSAGE, pushCanMessage);
    Cmd::setCommand(COMMAND_TOGGLE_CANBUS_SNIFF, toggleCanbusSniffer);
    Cmd::setCommand(COMMAND_SEND_ECHO, sendEchoMessage);
    Cmd::setCommand(COMMAND_TOGGLE_REVERSE, toggleMotorDirection);
    Cmd::setCommand(COMMAND_PRINT_LOOKUP, Logging::printLookup);
    Cmd::setCommand(COMMAND_UPDATE_SERIALVAR, SerialVar::receiveSerialVar);
    Heartbeat::beginReceiving();

#ifdef CONF_ECU_DEBUG
    Mirror::setup();
#endif

    TVAggression = 1.8f;
    static bool hasBeat = false;

    Log.d(ID, "Delaying 2 sec");
    Serial.flush();
    blinkStart();
    Pins::setInternalValue(PINS_INTERNAL_SYNC, 1);
    blinkStart();
    Serial.flush();

#if ECU_TESTING == FRONT_ECU
    full_front_test();
#endif

    while (!Heartbeat::checkBeat()) {
        delay(500);
        Cmd::receiveCommand();
    }

    while (true) {
        if (timeElapsedHigh >= INTERVAL_HIGH_PRIORITY) { // High priority updates
            timeElapsedHigh = 0;
            Cmd::receiveCommand();
            Log(ID, "Current Motor Speed:", MC::motorSpeed(), true);
            Log.d(ID, "Motor 0 Speed", MC::motorSpeed(0), true);
            Log.d(ID, "Motor 1 Speed", MC::motorSpeed(1), true);
            int pedal0, pedal1;
            Log.i(ID, "Brake", Pins::getPinValue(PINS_FRONT_BRAKE), true);
            Log.i(ID, "Steering", Pins::getPinValue(PINS_FRONT_STEER), true);
            Log.i(ID, "Pedal 0", (pedal0 = Pins::getPinValue(PINS_FRONT_PEDAL0)), true);
            Log.i(ID, "Pedal 1", (pedal1 = Pins::getPinValue(PINS_FRONT_PEDAL1)), true);
            Log.d(ID, "Pedal AVG", (pedal0 + pedal1) / 2, true);
            updateCurrentState();
            hasBeat = Heartbeat::checkBeat();
        }
        if (timeElapsedLow >= INTERVAL_MED_LOW_PRIORITY) { // Low priority updates
            timeElapsedLow = 0;

            static bool on = false;
            if (hasBeat && (currentState == &ECUStates::Idle_State)) {
                on = !on;
            } else {
                on = Pins::getCanPinValue(PINS_INTERNAL_START);
            }
            Log(ID, "Start Light", on, true);
            Pins::setPinValue(PINS_FRONT_START_LIGHT, on);

            if (Fault::anyFault()) {
                Fault::logFault();
            }

            Pins::setInternalValue(PINS_INTERNAL_TVAGG, TVAggression * 10000);

            Pins::setPinValue(PINS_FRONT_BMS_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_BMS_FAULT));
            Pins::setPinValue(PINS_FRONT_IMD_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_IMD_FAULT));

            logValues();
        }
    }
}

} // namespace Front