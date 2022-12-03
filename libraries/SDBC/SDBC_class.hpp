#include <stdint.h>

#include "Canbus.h"
#include "Pins.h"

namespace SDBC {

class MSG;

struct GPIO {
    const uint16_t pin;
    const bool outOrIn;
    const bool digOrAnl;
};
using VIRT = GPIO;

const struct SIG {
    const uint64_t bitMask;
    const MSG *msg;
    const GPIO *setter;
    const dt dataType;
    const uint8_t bitPos;
    const uint8_t nodeID;
    // TODO: Format function pointer
};

class MSG {
    CAN::Buffer buffer;

public:
    const bool outgoing;
    volatile union value {
        volatile uint8_t arr[8];
        volatile uint64_t raw;
    } value;

    void setSig(SIG signal, uint64_t data) {
        data <<= signal.bitPos;
        data &= signal.bitMask;

        buffer.lock_wait();
        value.raw &= ~signal.bitMask;
        value.raw |= data;
        buffer.unlock();
    }

    MSG(const uint32_t msgAddr, bool outgoing) : buffer(msgAddr, value.arr, outgoing), outgoing(outgoing) {}
};

} // namespace SDBC