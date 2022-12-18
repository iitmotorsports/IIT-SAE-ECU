#pragma once
#include <stdint.h>

#include "Canbus.h"
#include "Pins.h"

namespace SDBC {

class MSG;
struct SIG;

struct GPIO {
    const uint16_t pin;
    const SIG *settee;
    const bool outOrIn;
    const bool digOrAnl;
    const bool isVirt = false;
};

struct VIRT : GPIO {
};

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
    CAN::Buffer buffer; // FIXME: get live buffer from CAN lib

public:
    const bool outgoing;
    volatile union value {
        volatile uint8_t arr[8];
        volatile uint64_t raw;
    } value;
    const SIG **SIGNALS;
    const int signalCount;
    const bool hasValueSetter;

    template <typename T>
    void setSig(const SIG *signal, T value) { // IMPROVE: add specialization for double and float to ensure value is not mangled
        uint64_t data = *((uint64_t *)&value) & (~(uint64_t)0 >> (64 - 8 * sizeof(T)));
        data <<= signal->bitPos;
        data &= signal->bitMask;

        buffer.lock_wait();
        value.raw &= ~signal->bitMask;
        value.raw |= data;
        buffer.unlock();
    }

    template <typename T>
    T getSig(const SIG *signal) {
        uint64_t data = value.raw;
        data &= signal.bitMask;
        data >>= signal.bitPos;
        return *(T *)&data;
    }

    MSG(const uint32_t msgAddr, bool outgoing, const SIG **SIGNALS, const int signalCount, const bool hasValueSetter) : buffer(msgAddr, value.arr, outgoing), outgoing(outgoing), SIGNALS(SIGNALS), signalCount(signalCount), hasValueSetter(hasValueSetter) {}
};

struct LINK : MSG {
};

} // namespace SDBC