#pragma once
#include <stdint.h>

#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "Log.h"

#include "SDBC_class.hpp"
#include "SDBC_gen.hpp"

namespace SDBC {

LOG_TAG ID = "SDBC";

namespace Pins { // TODO: replace Pin lib API w/ this

const gpio_val_t gpio_values[ACTIVE_NODE::gpioMax];
const virt_val_t virt_values[ACTIVE_NODE::virtCount];

template <typename T>
void __set(const GPIO &pin, T value) {
    if (pin.pin >= VIRT_ID_OFFSET) { // NOTE: setting input virt allowed
        virt_values[pin.pin - VIRT_ID_OFFSET] = value;
    } else if (pin.outOrIn) {
        if (pin.pin < ACTIVE_NODE::gpioMax)
            gpio_values[pin.pin] = value;
        if (pin.digOrAnl)
            digitalWriteFast(pin.pin, value);
        else
            analogWrite(pin.pin, value);
    }
}
template <typename T>
inline void checkSettee(const SDBC::SIG *settee, T value) {
    if (settee != nullptr) { // has a value setter
        settee->msg->setSig(settee, value));
        // TODO: trigger CAN message to be sent
    }
}
template <typename T = gpio_val_t>
void set(const GPIO &pin, T value) {
    // FIXME: check if value is out of range than capable analog resolution? clamp to min max?
    checkSettee<T>(pin.settee, value);
    __set<T>(pin, value);
}
template <typename T = virt_val_t>
void set(const VIRT &pin, T value) {
    checkSettee<T>(pin.settee, value);
    __set<T>(pin, value);
}
template <typename T = gpio_val_t>
void set(const int pin, T value) { // Discourage direct pin access
    auto iter = ACTIVE_NODE.__pinMap.find(pin);
    if (iter != ACTIVE_NODE.__pinMap.end()) { // Check if pin is mapped
        SDBC::GPIO *gpio = iter->second;
        if (gpio->isVirt)
            return set(*(VIRT *)gpio, value);
        return set<T>(*gpio, value);
    }
    if (pin < VIRT_ID_OFFSET) { // VIRTs can only be set by reference, not indirectly
        if (pin < ACTIVE_NODE::gpioMax)
            gpio_values[pin.pin] = value;
    }
}

template <typename T = int>
T __get(const GPIO &pin) { // NOTE: get allowed regardless of input or output

    if (pin.pin >= VIRT_ID_OFFSET) {
        return virt_values[pin.pin - VIRT_ID_OFFSET];
    }

    if (pin.pin >= ACTIVE_NODE::gpioMax) {
        Log.e(ID, "Attempting to read a pin that is out of range", pin.pin);
        return 0;
    }

    if (pin.outOrIn) { // TODO: print verbose when "reading" output value
        return gpio_values[pin.pin];
    }

    if (pin.digOrAnl) {
        return (gpio_values[pin.pin] = digitalReadFast(pin.pin, value));
    }

    return (gpio_values[pin.pin] = analogRead(pin.pin, value));
}
template <typename T = gpio_val_t>
inline T get(const GPIO &pin) {
    return __get<T>(pin);
}
template <typename T = virt_val_t>
inline T get(const VIRT &pin) {
    return __get<T>(pin);
}
} // namespace Pins

// TODO: call upon receiving CAN message, will replace current callback
void incoming(const uint32_t addr, volatile uint8_t *buf) { // NOTE: currently assuming all 8 bytes are sent
    auto iter = MessageMap.find(addr);
    if (iter == MessageMap.end()) // Check if address is relevant
        return;

    auto msg = iter->second;
    if (msg->outgoing) // Check if message is actually incoming
        return;

    msg->value.raw = *(volatile uint64_t *)buf; // Update buffer

    if (!msg->hasValueSetter)
        return;

    for (size_t i = 0; i < msg->signalCount; i++) {
        auto sig = msg->SIGNALS[i];
        if (sig->setter == nullptr)
            continue;
        switch (sig->dataType) {
#define X(dt_id, typ)                                          \
    case dt::dt_id:                                            \
        Pins::__set<typ>(*sig->setter, msg->getSig<typ>(sig)); \
        break;
            SDBC_TYPE_DEFINITIONS
#undef X
        default:
            break;
        }
    }
}

// TODO: add option to have messages with value setters to be polled instead, not event driven like with Pins set. Sig will have setter defined versus settee defined in GPIO itself
void update() {
}

/* OLD */

// template <typename T>
// void post(const SIG signal, T data) {
//     if (ACTIVE_NODE.nodeID == signal.nodeID) { // Signal is sent by this node
//         signal.msg->setSig(signal, *((uint64_t *)&data));
//     } else if (!signal.msg->outgoing) { // Signal is received by a node
//         signal.msg->setSig(signal, *((uint64_t *)&data));
//         // TODO: immediately send entire message here on CAN line or setup to be sent
//     } else {
//         Log.w(ID, "Posting signal not relevant to node", signal.nodeID);
//     }
// }

// template <typename T>
// constexpr T get(const SIG signal) {
//     uint64_t data = signal.msg->value.raw;
//     data &= signal.bitMask;
//     data >>= signal.bitPos;
//     return *(T *)&data;
// }

// // TODO: call upon pin update, check if it matches a value setter

// // TODO: call upon setting a pin value (GPIO/VIRT)
// void updatePin(const uint32_t pin) {
//     auto iter = ACTIVE_NODE.ValueSetMap.find(pin);
//     if (iter != ACTIVE_NODE.ValueSetMap.end()) { // Check if pin is a value setter
//         auto signal = iter->second;

//         Pins::set(ACTIVE_NODE.PIN.ANALOG_TEST, 45);
//         Pins::set(ACTIVE_NODE.PIN.START_LED, 45);
//     }
// }

// // TODO: call upon relevant message update from CAN line
// void update(const SIG signal) {
//     if (signal.setter != nullptr)
//         if (signal.msg->outgoing) {
//             switch (signal.dataType) {
// // TODO: ensure setPin can set VIRT pins to any value type, not just integers
// // FIXME: use template to set/get pin values to include VIRTs? integer by default
// #define X(dt_id, typ)                                            \
//     case dt::dt_id:                                              \
//         Pins::setPinValue(signal.setter->pin, get<typ>(signal)); \
//         break;
//                 SDBC_TYPE_DEFINITIONS
// #undef X
//             default:
//                 break;
//             }
//         } else {
//             post(signal, Pins::getPinValue(signal.setter->pin));
//         }
// }

} // namespace SDBC