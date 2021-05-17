/**
 * @file SerialCommand.cpp
 * @author IR
 * @brief SerialCommand source file
 * @version 0.1
 * @date 2021-03-19
 * 
 * @copyright Copyright (c) 2021
 * 
 */

// @cond
#include "SerialCommand.h"
#include "Log.h"

namespace Cmd {

static LOG_TAG ID = "Serial Command";

#define X(value) value,
static const size_t COMMAND_COUNT = PP_NARG_MO(SERIAL_COMMANDS);
static CommandCallback callbacks[COMMAND_COUNT];
static size_t cmdMap[COMMAND_COUNT] = {SERIAL_COMMANDS};
#undef X

static size_t matchCommand(uint8_t val) {
    size_t i = 0;
#define X(value)        \
    if (value == val) { \
        return i;       \
    }                   \
    i++;
    SERIAL_COMMANDS
#undef X
    return COMMAND_COUNT;
}

int receiveCommand(void) {
    int serialData = -1;
    if (Serial.available()) {
        serialData = Serial.read();
        Log.d(ID, "Data received: ", serialData);
        size_t i;
        if ((i = matchCommand(serialData)) != COMMAND_COUNT && callbacks[i]) {
            callbacks[i]();
        }
    }
    return serialData;
}

void setCommand(uint8_t command, CommandCallback callback) {
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if (cmdMap[i] == command) {
            callbacks[i] = callback;
            Log.i(ID, "Command set for:", command);
            return;
        }
    }
    Log.w(ID, "No Command found for command:", command);
}
} // namespace Cmd
  // @endcond