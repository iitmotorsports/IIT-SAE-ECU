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
#include "Handlers.hpp"
#include "IntervalTimer.h"
#include "core_pins.h"

#include "PinConfig.def"

static int A_GPIO[CORE_NUM_TOTAL_PINS]; // IMPROVE: Use CORE_NUM_ANALOG instead

#define __WRITEPIN_DIGITAL(PIN, VAL) digitalWriteFast(PIN, VAL);
#define __WRITEPIN_ANALOG(PIN, VAL) analogWrite(PIN, VAL);
#define __READPIN_DIGITAL(PIN) digitalReadFast(PIN);
#define __READPIN_ANALOG(PIN) A_GPIO[PIN];

#define __INTERNAL_READ_ANALOG(PIN) A_GPIO[PIN] = analogRead(PIN);
#define __INTERNAL_READ_DIGITAL(PIN)

int Pins::getPinValue(uint8_t GPIO_Pin) {
    if (GPIO_Pin > CORE_NUM_TOTAL_PINS) {
        return 0;
#define X(pin, Type, IO)        \
    }                           \
    else if (GPIO_Pin == pin) { \
        return __READPIN_##Type(GPIO_Pin);
        TEENSY_PINS
#undef X
    } else {
        return 0;
    }
}

void Pins::setPinValue(uint8_t GPIO_Pin, int value) {
    if (GPIO_Pin > CORE_NUM_TOTAL_PINS) {
        return;
#define X(pin, Type, IO)        \
    }                           \
    else if (GPIO_Pin == pin) { \
        return __WRITEPIN_##Type(GPIO_Pin, value);
        TEENSY_PINS
#undef X
    } else {
        return;
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