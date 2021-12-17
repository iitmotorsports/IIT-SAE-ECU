/**
 * @file SerialVar.cpp
 * @author IR
 * @brief SerialVar Source File
 * @version 0.1
 * @date 2021-11-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

// @cond

#include "SerialVar.h"
#include "ECUGlobalConfig.h"
#include "PPHelp.h"

namespace SerialVar {

LOG_TAG LOG_ID = "SerialVar";

// The largest ID number defined
static const size_t MAX_ID = (
#define X(type, ID) max(ID,
    SERIALVARS
#undef X
    - 1
#define X(type, ID) )
    SERIALVARS
#undef X
);

static uint8_t variables[MAX_ID + 1][8];

uint8_t *getVariable(size_t ID) {
    return variables[ID];
}

void updateVariable(size_t varID, const uint8_t *dataArr) {
    switch (varID) {
#define X(type, ID)                                                                 \
    case ID:                                                                        \
        memcpy(variables[ID], dataArr, 8);                                          \
        Log.i(LOG_ID, "Data received for varID:", ID);                              \
        Log.i(LOG_ID, "Int value:", *(int *)(variables + ID));                      \
        Log.i(LOG_ID, "Approximate Float value:", (int)*(float *)(variables + ID)); \
        break;
        SERIALVARS
#undef X
    default:
        Log.i(LOG_ID, "Invalid varID:", varID);
    }
}

void receiveSerialVar() {
    static elapsedMillis timeElapsed;
    static uint8_t buffer[9];

    timeElapsed = 0;
    int c = -1;

    Log.i(LOG_ID, "Waiting for data");

    while (c < 8) {
        if (Serial.available()) {
            buffer[++c] = Serial.read();
        } else if (timeElapsed > 1500) { // This process should be quick
            Log.w(LOG_ID, "Timeout waiting for data");
            return;
        }
    }

    uint8_t varID = buffer[0];

    updateVariable(varID, buffer + 1);
}

} // namespace SerialVar
  // @endcond