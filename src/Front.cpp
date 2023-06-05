#include "Front.h"
#include "ECUGlobalConfig.h"
#include "Heartbeat.h"
#include "unordered_map"

namespace Front {

LOG_TAG ID = "Front Teensy";

static elapsedMillis timeElapsed;
static elapsedMillis blinkTimeElapsed;

void blinkStart() {
    Pins::setPinValue(PINS_FRONT_START_LIGHT, 1);
    delay(100);
    Pins::setPinValue(PINS_FRONT_START_LIGHT, 0);
    delay(100);
}

void run() {
    Log.i(ID, "Hawkrod: Initializing Front ECU ...");

    Canbus::setup(); // allocate and organize addresses
    Pins::initialize(); // setup predefined pins
    loadStateMap();

    Heartbeat::beginBeating();
    Heartbeat::beginReceiving();

    blinkStart();

    static bool tempBlink = false;

    while (true) {
        if(Heartbeat::checkBeat()) {
            if (timeElapsed >= INTERVAL_HIGH_PRIORITY) { // High priority updates
                timeElapsed = 0;

                updateCurrentState();
                
                // Set fault LEDs
                Pins::setPinValue(PINS_FRONT_BMS_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_BMS_FAULT));
                Pins::setPinValue(PINS_FRONT_IMD_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_IMD_FAULT));

                updateStartLight();
            }
        } else {
            if (blinkTimeElapsed >= INTERVAL_LED_BLINK) { // High priority updates
                blinkTimeElapsed = 0;

                // Blink fault LEDs to identify no connection
                tempBlink = !tempBlink;
                Pins::setPinValue(PINS_FRONT_BMS_LIGHT, tempBlink);
                Pins::setPinValue(PINS_FRONT_IMD_LIGHT, tempBlink);
            }
        }
    }
}

} // namespace Front