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

#include "Log.h"
#include "PinConfig.def"

static int A_GPIO[CORE_NUM_TOTAL_PINS]; // IMPROVE: Use CORE_NUM_ANALOG instead
#define X ,
static int CAN_GPIO[PP_NARG_MO(CANBUS_PINS)]; // Number of pins to be read through canbus
#undef X

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

static const LOG_TAG ID = "Pins";

int Pins::getPinValue(uint8_t GPIO_Pin) {
    if (GPIO_Pin > CORE_NUM_TOTAL_PINS) {
        Log.e(ID, "Acessing out of range pin", GPIO_Pin);
        return 0;
#define X(pin, Type, IO) __READPIN_##Type##IO(pin);
        TEENSY_PINS
#undef X
    } else {
        Log.d(ID, "No pin defined", GPIO_Pin);
        return 0;
    }
}

void Pins::setPinValue(uint8_t GPIO_Pin, int value) {
    if (GPIO_Pin > CORE_NUM_TOTAL_PINS) {
        return;
#define X(pin, Type, IO) __WRITEPIN_##Type##IO(pin, value);
        TEENSY_PINS
#undef X
    }
}

void Pins::update(void) {
#define X(pin, Type, IO) __INTERNAL_READ_##Type(pin);
    TEENSY_PINS
#undef X
}

void Pins::initialize(void) {
#define X(pin, Type, IO) pinMode(pin, IO);
    TEENSY_PINS
#undef X
}

// @endcond