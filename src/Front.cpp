#include "Front.h"
#include "ECUGlobalConfig.h"
#include "Heartbeat.h"
#include "unordered_map"
#if ECU_TESTING == FRONT_ECU
#include "Test.h"
#endif

namespace Front {

LOG_TAG ID = "Front Teensy";

static elapsedMillis timeElapsed;

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

void blinkStart() {
    Pins::setPinValue(PINS_FRONT_START_LIGHT, 1);
    delay(100);
    Pins::setPinValue(PINS_FRONT_START_LIGHT, 0);
    delay(100);
}

void LEDBlink() {
    Pins::setPinValue(LED_BUILTIN, 0);
    delay(100);
    Pins::setPinValue(LED_BUILTIN, 1);
    delay(100);
    Pins::setPinValue(LED_BUILTIN, 0);
    delay(100);
    Pins::setPinValue(LED_BUILTIN, 1);
    delay(100);
    Pins::setPinValue(LED_BUILTIN, 0);
}

void run() {
    Log.i(ID, "Teensy 4.1 SAE FRONT ECU  initializing");

    Log.i(ID, "Setting up Canbus");
    Canbus::setup(); // allocate and organize addresses
    Log.i(ID, "Initalizing Pins");
    Pins::initialize(); // setup predefined pins
    LEDBlink();
    LEDBlink();
    loadStateMap();

    Heartbeat::beginBeating();
    Heartbeat::beginReceiving();

    static bool hasBeat = false;

    // Serial.flush();
    blinkStart();
    Pins::setInternalValue(PINS_INTERNAL_SYNC, 1);
    blinkStart();
    // Serial.flush();

#if ECU_TESTING == FRONT_ECU
    full_front_test();
#endif

    Log.i(ID, "Waiting for heartbeat");
    while (!Heartbeat::checkBeat()) {
        delay(500);
    }

    while (true) {
        if (timeElapsed >= INTERVAL_HIGH_PRIORITY) { // High priority updates
            timeElapsed = 0;

            updateCurrentState();
            hasBeat = Heartbeat::checkBeat();
            
            // Set fault LEDs
            Pins::setPinValue(PINS_FRONT_BMS_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_BMS_FAULT));
            Pins::setPinValue(PINS_FRONT_IMD_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_IMD_FAULT));

            faultBlink();
            updateStartLight(hasBeat);
        }
    }
}

} // namespace Front