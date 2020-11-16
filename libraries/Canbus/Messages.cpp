/**
 * @file Messages.cpp
 * @author IR
 * @brief Messages source file
 * @version 0.1
 * @date 2020-11-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "Messages.hpp"

namespace CanbusMsg {

static const uint64_t BMS_HARDFAULTMASK = CONF_BITMASK_BMS_HARDFAULT;
static const uint64_t RMS_HARDFAULTMASK = CONF_BITMASK_RMS_HARDFAULT;

static inline int BMS_HardFaultCheck(const uint8_t raw_data[8]) {
    return ((*(uint64_t *)raw_data) & BMS_HARDFAULTMASK) == 0;
}

static inline int RMS_HardFaultCheck(const uint8_t raw_data[8]) {
    return ((*(uint64_t *)raw_data) & RMS_HARDFAULTMASK) == 0;
}

void SerialPrintMessage(const CAN_message_t &msg) {
    uint8_t *buf = new uint8_t[10](); // IMPROVE: Check whether buffering is better then separate calls to Serial.write
    //TODO: Verify that id pointer does not have to be shifted
    memcpy(buf, &msg.id, 2); // NOTE: will cut out can messages that are larger than 2 bytes
    memcpy(buf + 2, &msg.buf, 8);
    Serial.write(buf, 10);
    delete[] buf;
}

void BMSFaultMsg(const CAN_message_t &msg) {
    if (BMS_HardFaultCheck(msg.buf)) {     // HardFault triggered
        Serial.println("BMS HARD FAULT!"); // Not actual implementation | Call interrupt?
    }
}

void RMSFaultMsg(const CAN_message_t &msg) {
    if (RMS_HardFaultCheck(msg.buf)) {     // HardFault triggered
        Serial.println("RMS HARD FAULT!"); // Not actual implementation | Call interrupt?
    }
}

} // namespace CanbusMsg