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
#include "PPHelp.h"

namespace SerialVar {

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

static int varIndex[MAX_ID + 1] = {
#define X(type, ID) ID,
    SERIALVARS
#undef X
};

static SerialVarObj_t variables[MAX_ID + 1] = {
#define X(type, ID) SerialVar<type>(),
    SERIALVARS
#undef X
};

SerialVarObj_t getVariable(size_t ID) {
    return variables[varIndex[ID]];
}

void updateVariable(size_t varID, byte *dataArr, int count) {
    if (varID > MAX_ID) {
        Log.i(LOGID, "Invalid varID:", varID);
        return;
    }
    // getVariable(varID).update(dataArr, count);
}

void receiveSerialVar() {
    static elapsedMillis timeElapsed;
    static byte buffer[32];

    timeElapsed = 0;
    int c = 0;

    Log.i(LOGID, "Waiting for data");

    while (c < 32) {
        if (Serial.available() && (buffer[c++] = Serial.read()) == SERIALVAR_STOP_RECEIVE) {
            c--;
            break;
        } else if (timeElapsed > 8000) { // This process should be quick
            Log.w(LOGID, "Timedout waiting for data");
            return;
        }
    }

    byte varID = buffer[0];
    byte size = buffer[1];

    Log.i(LOGID, "Data Received for varID:", varID);

    updateVariable(varID, buffer + 2, size);
}

} // namespace SerialVar
  // @endcond