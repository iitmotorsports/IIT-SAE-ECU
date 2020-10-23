#include "Pins.h"
#include "Handlers.hpp"

#include "PinConfig.def"

static elapsedMillis timeElapsed;
static int pos = 0;

// TODO: ditch XMacros to allow pins to be set dynamiclly, in or out and only if a state requires it

#define X(...) ,
static const int pinBlocking = sqrt(PP_NARG_MO(TEENSY_PINS)); // Gets the number of pins to poll every update
static const int pinCount = PP_NARG_MO(TEENSY_PINS);          // Length of the pin array
static const int pinDelay = CONF_POLLING_DELAY;                  // Milliseconds between the time the teensy polls a chunk of pins
#undef X

#define X(pin, Type, IO) {pin, Type##IO},
static Pins::pin_t pins[pinCount] = {TEENSY_PINS}; // Allocate pins
#undef X

// ALT: allocate all GPIO pins so index matches GPIO number
inline static Pins::pin_t *getPin(const int GPIO_Pin) {
    int i = pinCount;
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

int Pins::getPinValue(const int GPIO_Pin) {
    return getPin(GPIO_Pin)->value;
}

void Pins::setPinValue(const int GPIO_Pin, const int value) {
    getPin(GPIO_Pin)->value = value;
}

void Pins::update(void) {
    if (timeElapsed >= pinDelay) {
        timeElapsed = timeElapsed - pinDelay;
        for (size_t i = 0; i < pinBlocking; i++) {
            ++pos;
            pos = pos % pinCount;
            pins[pos].update();
        }
    }
}

void Pins::initialize(void) {
#define X(pin, Type, IO) pinMode(pin, IO);
    TEENSY_PINS
#undef X
}