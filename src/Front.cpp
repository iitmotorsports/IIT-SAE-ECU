#include "Front.h"
#include "ECUGlobalConfig.h"
#include "Heartbeat.h"
#include "Util.h"
#include "unordered_map"

namespace Front {

LOG_TAG ID = "Front Teensy";

static elapsedMillis timeElapsed;

void run() {
    Log.i(ID, "Hawkrod: Initializing Front ECU ...");

    Canbus::setup(); // allocate and organize addresses
    Pins::initialize(); // setup predefined pins
    loadStateMap();

    Heartbeat::beginBeating();
    Heartbeat::beginReceiving();

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
            Pins::setPinValue(PINS_FRONT_BMS_LIGHT, getGlobalBlinkState());
            Pins::setPinValue(PINS_FRONT_IMD_LIGHT, getGlobalBlinkState());
            Pins::setPinValue(PINS_FRONT_START_LIGHT, getGlobalBlinkState());
        }
    }
}

} // namespace Front