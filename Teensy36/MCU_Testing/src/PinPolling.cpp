#include <stdint.h>
#include <stdlib.h>

#include "PinPolling.h"
#include "config.def"

static elapsedMillis timeElapsed;
static int pos = 0;

#define X(pin, IO, Handle) {pin, Handle},
static Pins::pin_t pins[Pins::pinCount] = {TEENSY_PINS}; // Allocate pins
#undef X

inline static Pins::pin_t *getPin(const int GPIO_Pin) {
    int i = Pins::pinCount;
    switch (GPIO_Pin) {
#define X(pin, ...) \
    case pin:       \
        i--;

        TEENSY_PINS
#undef X
    default:
        break;
    }
    return &pins[i];
}

extern int Pins::getPinValue(const int GPIO_Pin) {
    return getPin(GPIO_Pin)->value;
}

extern void Pins::setPinValue(const int GPIO_Pin, const int value) {
    getPin(GPIO_Pin)->value = value;
}

extern void Pins::update(void) {
    if (timeElapsed >= delay) {
        timeElapsed = timeElapsed - delay;
        for (size_t i = 0; i < pinBlocking; i++) {
            ++pos;
            pos = pos % pinCount;
            pins[pos].update();
        }
    }
}

extern void Pins::initialize(void) {
#define X(pin, IO, ...) pinMode(pin, IO);
    TEENSY_PINS
#undef X
}