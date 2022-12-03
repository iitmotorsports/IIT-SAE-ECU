#include <stdint.h>

#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "Log.h"

#include "SDBC_class.hpp"
#include "SDBC_gen.hpp"

namespace SDBC {

LOG_TAG ID = "SDBC";

template <typename T>
void post(const SIG signal, T data) {
    if (ACTIVE_NODE.nodeID == signal.nodeID) { // Signal is sent by this node
        signal.msg->setSig(signal, *((uint64_t *)&data));
    } else if (!signal.msg->outgoing) { // Signal is received by a node
        signal.msg->setSig(signal, *((uint64_t *)&data));
        // TODO: immediately send entire message on CAN line or setup to be sent
    } else {
        Log.w(ID, "Posting signal not relevant to node", signal.nodeID);
    }
}

template <typename T>
constexpr T get(const SIG signal) {
    uint64_t data = signal.msg->value.raw;
    data &= signal.bitMask;
    data >>= signal.bitPos;
    return *(T *)&data;
}

// TODO: call upon relevant message update from CAN line
void update(const SIG signal) {
    if (signal.setter != nullptr)
        if (signal.msg->outgoing) {
            switch (signal.dataType) {
// TODO: ensure setPin can set VIRT pins to any value type, not just integers
#define X(dt_id, typ)                                            \
    case dt::dt_id:                                              \
        Pins::setPinValue(signal.setter->pin, get<typ>(signal)); \
        break;
                SDBC_TYPE_DEFINITIONS
#undef X
            default:
                break;
            }
        } else {
            post(signal, Pins::getPinValue(signal.setter->pin));
        }
}

void test() {
    auto a = get<int32_t>(FRONT_ECU.DEBUG_INTEGER);
}

} // namespace SDBC