#include "Front.h"
#include "ECUGlobalConfig.h"
#include "Heartbeat.h"
#include "Mirror.h"
#include "SerialCommand.h"
#include "SerialVar.h"
#include "unordered_map"
#if ECU_TESTING == FRONT_ECU
#include "Test.h"
#endif

namespace Front {

LOG_TAG ID = "Front Teensy";

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
    // Echo::echo(delay, address, (uint8_t *)buffer);
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
    Cmd::setCommand(COMMAND_ENABLE_CHARGING, setChargeSignal);         // TODO: RIP
    Cmd::setCommand(COMMAND_SEND_CANBUS_MESSAGE, pushCanMessage);      // TODO: RIP
    Cmd::setCommand(COMMAND_TOGGLE_CANBUS_SNIFF, toggleCanbusSniffer); // TODO: RIP
    Cmd::setCommand(COMMAND_SEND_ECHO, sendEchoMessage);               // TODO: RIP
    Cmd::setCommand(COMMAND_TOGGLE_REVERSE, toggleMotorDirection);
    Cmd::setCommand(COMMAND_PRINT_LOOKUP, Logging::printLookup); // TODO: RIP
    Cmd::setCommand(COMMAND_UPDATE_SERIALVAR, SerialVar::receiveSerialVar);
    Heartbeat::beginReceiving();

#ifdef CONF_ECU_DEBUG
    Mirror::setup();
#endif

    TVAggression = 1.8f;
    static int lastBeat = 0;

    Log.d(ID, "Delaying 2 sec");
    Serial.flush();
    blinkStart();
    Pins::setInternalValue(PINS_INTERNAL_SYNC, 1);
    blinkStart();
    Serial.flush();

#if ECU_TESTING == FRONT_ECU
    full_front_test();
#endif
    Heartbeat::beginBeating();
    while (!Heartbeat::checkBeat()) {
        delay(500);
        Cmd::receiveCommand();
    }

    while (true) {
        if (timeElapsedHigh >= INTERVAL_HIGH_PRIORITY) { // High priority updates
            timeElapsedHigh = 0;
            Cmd::receiveCommand();

            highPriorityValues();

            updateCurrentState();
            lastBeat = Heartbeat::checkBeat();
        }
        if (timeElapsedMidHigh >= INTERVAL_MED_HIGH_PRIORITY) { // Low priority updates
            timeElapsedMidHigh = 0;

            Pins::setPinValue(PINS_FRONT_BMS_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_BMS_FAULT));
            Pins::setPinValue(PINS_FRONT_IMD_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_IMD_FAULT));

            medPriorityValues(lastBeat);
        }
        if (timeElapsedLow >= INTERVAL_MED_LOW_PRIORITY) { // Low priority updates
            timeElapsedLow = 0;

            updateStartLight(lastBeat);

            if (Fault::anyFault()) {
                Fault::logFault();
            }

            Pins::setInternalValue(PINS_INTERNAL_TVAGG, TVAggression * 10000);

            lowPriorityValues();
        }
    }
}

} // namespace Front