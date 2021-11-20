/**
 * @file Faults.cpp
 * @author IR
 * @brief Faults source file
 * @version 0.1
 * @date 2021-01-27
 * 
 * @copyright Copyright (c) 2021
 *
 */

// @cond

#include "Faults.h"
#include "Canbus.h"
#include "FaultConfig.def"
#include "Log.h"
#include "Pins.h"

namespace Fault {

static LOG_TAG ID = "Fault Check";

#define X(...) ,
static const int HARD_CAN_COUNT = PP_NARG_MO(HARD_FAULT_ADD);
static const int SOFT_CAN_COUNT = PP_NARG_MO(SOFT_FAULT_ADD);
static const int CAN_COUNT = PP_NARG_MO(HARD_FAULT_ADD) + PP_NARG_MO(SOFT_FAULT_ADD); // Number of Can addresses we need to account for

static const int HARD_PIN_COUNT = PP_NARG_MO(HARD_PIN_FAULTS);
static const int SOFT_PIN_COUNT = PP_NARG_MO(SOFT_PIN_FAULTS);
static const int PIN_COUNT = PP_NARG_MO(HARD_PIN_FAULTS) + PP_NARG_MO(SOFT_PIN_FAULTS); // Number of Pins we need to account for
#undef X

typedef struct CanFault {
    uint32_t address;
    Canbus::Buffer buffer;
    uint64_t faultMask;
    LOG_MSG *tags;
    bool faulted = false;
    union lastValue {
        uint8_t arr[8];
        uint64_t longlong;
    } lastValue;

    CanFault(const uint32_t address, const uint64_t faultMask) {
        this->address = address;
        this->buffer = address;
        this->faultMask = faultMask;
    }

    bool check() {
        uint64_t curr = buffer.getULong();
        if (curr & faultMask) {
            buffer.dump(lastValue.arr);
#ifdef CONF_ECU_DEBUG
            Log.w(ID, "Faulted CAN Address:", address);
#endif
            return faulted = true;
        }
        return faulted = false;
    }

    void clear() {
        buffer.clear();
        lastValue.longlong = 0;
        faulted = false;
    }

    void log() {
#define X(add, mask, id)                               \
    if (address == add && lastValue.longlong & mask) { \
        /*PRE_BUILD_IGNORE*/ Log.e(ID, id);            \
    }
        // Ignore Pre_Build error
        CAN_FAULT_IDS
#undef X
        clear();
    }
} CanFault;

typedef struct PinFault {
    int lastValue = 0;
    uint8_t GPIO_Pin;
    bool faulted = false;

    PinFault(const uint8_t GPIO_Pin) {
        this->GPIO_Pin = GPIO_Pin;
    }

    void clear() {
        lastValue = 0;
        faulted = false;
    }

    bool check() {
#define X(pin, comp, value, id) (GPIO_Pin == pin && Pins::getPinValue(pin) comp value) ||
        if (HARD_PIN_FAULTS SOFT_PIN_FAULTS 0) {
#ifdef CONF_ECU_DEBUG
            Log.w(ID, "Faulted pin:", GPIO_Pin);
#endif
            return faulted = true;
        }
#undef X
        return faulted = false;
    }

    void log() {
#define X(pin, comp, value, id)             \
    if (GPIO_Pin == pin && this->faulted) { \
        /*PRE_BUILD_IGNORE*/ Log.e(ID, id); \
    }
        HARD_PIN_FAULTS
        SOFT_PIN_FAULTS
#undef X
        clear();
    }
} PinFault;

static CanFault hardCanFaults[HARD_CAN_COUNT] = {
#define X(add, mask, tag) CanFault(add, mask),
    HARD_FAULT_ADD
#undef X
};

static CanFault softCanFaults[SOFT_CAN_COUNT] = {
#define X(add, mask, tag) CanFault(add, mask),
    SOFT_FAULT_ADD
#undef X
};

static PinFault hardPinFaults[HARD_PIN_COUNT] = {
#define X(pin, comp, value, id) PinFault(pin),
    HARD_PIN_FAULTS
#undef X
};

static PinFault softPinFaults[SOFT_PIN_COUNT] = {
#define X(pin, comp, value, id) PinFault(pin),
    SOFT_PIN_FAULTS
#undef X
};

bool hardFault(void) {
    bool faulted = false;

    for (size_t i = 0; i < HARD_CAN_COUNT; i++) {
        faulted |= hardCanFaults[i].check();
    }

    for (size_t i = 0; i < HARD_PIN_COUNT; i++) {
        faulted |= hardPinFaults[i].check();
    }

    return faulted;
}

bool softFault(void) {
    bool faulted = false;

    for (size_t i = 0; i < SOFT_CAN_COUNT; i++) {
        faulted |= softCanFaults[i].check();
    }

    for (size_t i = 0; i < SOFT_PIN_COUNT; i++) {
        faulted |= softPinFaults[i].check();
    }

    return faulted;
}

void logFault(void) {
    for (size_t i = 0; i < HARD_CAN_COUNT; i++) {
        hardCanFaults[i].log();
    }
    for (size_t i = 0; i < SOFT_CAN_COUNT; i++) {
        softCanFaults[i].log();
    }
    for (size_t i = 0; i < HARD_PIN_COUNT; i++) {
        hardPinFaults[i].log();
    }
    for (size_t i = 0; i < SOFT_PIN_COUNT; i++) {
        softPinFaults[i].log();
    }
}

bool anyFault(void) {
    return hardFault() || softFault();
}

} // namespace Fault
  // @endcond