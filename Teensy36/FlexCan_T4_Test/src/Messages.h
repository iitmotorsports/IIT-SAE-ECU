#include <stdint.h>
#include <stdlib.h>

#include "Fault.h"

static void BMSFaultMsg(const CAN_message_t &msg) {
    if (BMS_HardFaultCheck(msg.buf)) {     // HardFault triggered
        Serial.println("BMS HARD FAULT!"); // Not actual implementation | Call interrupt?
    }
}

static void RMSFaultMsg(const CAN_message_t &msg) {
    if (RMS_HardFaultCheck(msg.buf)) {     // HardFault triggered
        Serial.println("RMS HARD FAULT!"); // Not actual implementation | Call interrupt?
    }
}