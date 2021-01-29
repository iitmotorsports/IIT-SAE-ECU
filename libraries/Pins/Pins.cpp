/**
 * @file Pins.cpp
 * @author IR
 * @brief Pins source file
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */

// @cond

#include "Pins.h"
#include "IntervalTimer.h"
#include "core_pins.h"

#include "Canbus.h"
#include "Log.h"
#include "PinConfig.def"

namespace Pins {

static int A_GPIO[CORE_NUM_TOTAL_PINS]; // IMPROVE: Use CORE_NUM_ANALOG instead
#define X ,
static uint8_t CAN_GPIO_MAP[PP_NARG_MO(PINS_CANBUS_ANALOG) + PP_NARG_MO(PINS_CANBUS_DIGITAL)]; // Map actual pins to an array position
static int CAN_GPIO[PP_NARG_MO(PINS_CANBUS_ANALOG) + PP_NARG_MO(PINS_CANBUS_DIGITAL)];         // Store canpin values
static const uint activeDigitalCanPins = min(PP_NARG_MO(PINS_CANBUS_DIGITAL), 8);              // NOTE: MAX 8 Digital pins per msg
#undef X

static IntervalTimer canbusPinUpdate;
static const LOG_TAG ID = "Pins";

struct CanPinMsg {
    uint32_t address;
    uint8_t buf[8] = {0};
    uint64_t *bufmap = (uint64_t *)buf;
    void receive();
    void send();
};

struct digitalCanPinMsg : CanPinMsg {
    uint8_t digitalPins[activeDigitalCanPins];
    void send() {
        for (size_t i = 0; i < activeDigitalCanPins; i++) {
            *bufmap = *bufmap << 1;
            *bufmap |= (bool)getPinValue(digitalPins[i]);
        }
        Log.d(ID, "Sending Digital Pins", address);
        Canbus::sendData(address, buf);
    };
    void receive() {
        for (size_t i = 0; i < activeDigitalCanPins; i++) {
            *bufmap = *bufmap << 1;
            *bufmap |= (bool)getPinValue(digitalPins[i]);
        }
        Log.d(ID, "Sending Analog Pins", address);
    }
};

struct analogCanPinMsg : CanPinMsg {
    uint8_t AnalogPins[2] = {0};
};

#define __WRITEPIN_DIGITALOUTPUT(PIN, VAL) \
    }                                      \
    else if (GPIO_Pin == PIN) {            \
        return digitalWriteFast(PIN, VAL);

#define __WRITEPIN_ANALOGOUTPUT(PIN, VAL) \
    }                                     \
    else if (GPIO_Pin == PIN) {           \
        return analogWrite(PIN, VAL);

#define __READPIN_DIGITALINPUT(PIN) \
    }                               \
    else if (GPIO_Pin == PIN) {     \
        return digitalReadFast(PIN);

// TODO: actually implement analog value caching
#define __READPIN_ANALOGINPUT(PIN) \
    }                              \
    else if (GPIO_Pin == PIN) {    \
        return analogRead(PIN);    \
        // return A_GPIO[PIN];

#define __WRITEPIN_DIGITALINPUT(PIN, VAL)
#define __WRITEPIN_ANALOGINPUT(PIN, VAL)
#define __READPIN_DIGITALOUTPUT(PIN)
#define __READPIN_ANALOGOUTPUT(PIN)

#define __INTERNAL_READ_ANALOG(PIN) A_GPIO[PIN] = analogRead(PIN);
#define __INTERNAL_READ_DIGITAL(PIN)

static void _pushCanbusPins(void) {
}

int getPinValue(uint8_t GPIO_Pin) {
    if (GPIO_Pin > CORE_NUM_TOTAL_PINS) {
        Log.e(ID, "Acessing out of range pin", GPIO_Pin);
        return 0;
#define X(pin, Type, IO) __READPIN_##Type##IO(pin);
        ECU_PINS
#undef X
    } else {
        Log.d(ID, "No pin defined", GPIO_Pin);
        return 0;
    }
}

void setPinValue(uint8_t GPIO_Pin, int value) {
    if (GPIO_Pin > CORE_NUM_TOTAL_PINS) {
        return;
#define X(pin, Type, IO) __WRITEPIN_##Type##IO(pin, value);
        ECU_PINS
#undef X
    }
}

void update(void) {
#define X(pin, Type, IO) __INTERNAL_READ_##Type(pin);
    ECU_PINS
#undef X
}

void initialize(void) {
#define X(pin, Type, IO) pinMode(pin, IO);
    ECU_PINS
#undef X
#if PINS_CANBUS_DIRECTION == OUTPUT // Only need to update when pins are outgoing
    canbusPinUpdate.begin(_pushCanbusPins, CONF_PINS_CANBUS_UPDATE_INTERVAL_MICRO);
#endif
}

} // namespace Pins

// @endcond