/**
 * @file Mirror.cpp
 * @author IR
 * @brief Mirror source code
 * @version 0.1
 * @date 2021-03-30
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "Mirror.h"
#include "CanBus.h"
#include "ECUGlobalConfig.h"
#include "Log.h"
#include "Pins.h"
#include "SerialCommand.h"

namespace Mirror {

#define TIMER_RECEIVE_MICROS 2000

static LOG_TAG ID = "Mirror";
static IntervalTimer serialCheckTimer;
static bool cont = true;

static void toggleMirrorMode(void) {
    cont = !cont;
    // Canbus::sendData(ADD_MIRROR, cont);
    if (cont) {
        enterMirrorMode();
    } else {
        exitMirrorMode();
    }
}

// static void setMirrorMode(uint32_t val, volatile uint8_t *pnt) {
//     if (cont = pnt[0]) {
//         enterMirrorMode();
//     } else {
//         exitMirrorMode();
//     }
// }

void timerReceive() {
    Command::receiveCommand()
}

void setup(void) { // TODO: auto receive on back ECU using timer
#if CONF_ECU_POSITION == BACK_ECU
    serialCheckTimer.begin(timerReceive, TIMER_RECEIVE_MICROS);
#endif
    Command::setCommand(COMMAND_TOGGLE_MIRROR_MODE, toggleMirrorMode);
    // #else
    // Canbus::addCallback(ADD_MIRROR, setMirrorMode);
    // #endif
}

void enterMirrorMode(void) {
#if CONF_ECU_POSITION == BACK_ECU
    serialCheckTimer.end();
#endif
    cont = true;
    while (cont) {
        // #if CONF_ECU_POSITION == FRONT_ECU
        int pin = Command::receiveCommand();
        if (pin != -1) {
            Log(ID, "Requested pin", Pins::getPinValue(pin));
        }
        // #endif
    }
#if CONF_ECU_POSITION == BACK_ECU
    serialCheckTimer.begin(timerReceive, TIMER_RECEIVE_MICROS);
#endif
}

void exitMirrorMode(void) {
    cont = false;
}

} // namespace Mirror