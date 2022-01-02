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
//@cond

#include "Mirror.h"
#include "CanBus.h"
#include "ECUGlobalConfig.h"
#include "Heartbeat.h"
#include "Log.h"
#include "Pins.h"
#include "SerialCommand.h"

namespace Mirror {

#define TIMER_RECEIVE_MICROS 2000

static LOG_TAG ID = "Mirror";
static bool cont = false;
static bool init = false;
static bool receive = true;

static void toggleMirrorMode(void) {
    cont = !cont;
    if (cont) {
        enterMirrorMode();
    } else {
        exitMirrorMode();
    }
}

void timerReceive() {
    if (receive)
        Cmd::receiveCommand();
}

void setup(void) {
    if (!init) {
#if CONF_ECU_POSITION == BACK_ECU
        receive = true;
        Heartbeat::addCallback(timerReceive);
#endif
        Cmd::setCommand(COMMAND_TOGGLE_MIRROR_MODE, toggleMirrorMode);
    }
    init = true;
}

static int getSerialByte() {
    int serialData = 0;
    while (!Serial.available()) {
    }
    serialData = Serial.read();
    Log.d(ID, "Data received:", serialData);
    return serialData;
}

static int getSerialInt() {
    int serialData = 0;
    while (!Serial.available()) {
    }
    size_t count = Serial.readBytes((char *)(&serialData), 4);
    if (count != 4) {
        Log.w(ID, "Did not receive 4 bytes for integer", count);
    }

    Log.d(ID, "Data received:", serialData);
    return serialData;
}

void enterMirrorMode(void) {
#if CONF_ECU_POSITION == BACK_ECU
    receive = false;
#endif
    cont = true;
    Log(ID, "Mirror Mode Enabled");
    while (cont) { // NOTE: Low priority heartbeat will freeze because of this
        Log(ID, "Waiting for data");
        int serialData = getSerialByte();
        if (serialData == COMMAND_TOGGLE_MIRROR_MODE) {
            toggleMirrorMode();
        } else if (serialData == COMMAND_ENTER_MIRROR_SET) {
            Log(ID, "Waiting for a pin to set");
            int pin = getSerialByte();
            Log(ID, "Waiting for the value to set it to");
            int value = getSerialInt();
            Log(ID, "Setting pin:", pin);
            Log(ID, "To value:", value);
            Pins::setPinValue(pin, value);
        } else {
            Log(ID, "Requested pin", Pins::getPinValue(serialData));
        }
        // delay(50);
    }
    Log(ID, "Mirror Mode Disabled");
#if CONF_ECU_POSITION == BACK_ECU
    receive = true;
#endif
}

void exitMirrorMode(void) {
    cont = false;
}

} // namespace Mirror
  //@endcond