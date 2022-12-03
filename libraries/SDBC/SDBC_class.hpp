#include <stdint.h>

#include "Canbus.h"

namespace SDBC {

const struct SIG {
    const uint64_t bitMask;
    const uint32_t msgAddr;
    const uint8_t nodeID;
    const uint8_t bitPos;
    // TODO: Format function pointer
};

class MSG {
    CAN::Buffer buffer;
    const SIG *sigs;
    const int sigCount;

    volatile union value {
        volatile uint8_t arr[8];
        volatile uint64_t raw;
    } value;

public:
    void setSig(SIG signal, uint64_t data) {
        data <<= signal.bitPos;
        data &= signal.bitMask;

        buffer.lock_wait();
        value.raw &= ~signal.bitMask;
        value.raw |= data;
        buffer.unlock();
    }

    MSG(const uint32_t msgAddr, const SIG *sigs, const int sigCount) : buffer(msgAddr, value.arr, true), sigs(sigs), sigCount(sigCount) {}
};

} // namespace SDBC