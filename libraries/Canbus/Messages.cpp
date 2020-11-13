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

}