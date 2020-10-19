#ifndef __PINPOLLING_H__
#define __PINPOLLING_H__

#include <stdint.h>
#include <stdlib.h>

#include "Pinouts.def"
#include "config.def"

//IMPROVE: pin priority

namespace Pins {

typedef void (*PinHandler)(const int pin, int &value);

typedef struct pin_t {
    uint16_t GPIO;
    PinHandler handle;
    int value;

    void update(void) {
        handle(GPIO, value);
    }
} pin_t;

#define X(...) ,
static const int pinBlocking = sqrt(PP_NARG_MO(TEENSY_PINS)); // Gets the number of pins to poll every update
static const int pinCount = PP_NARG_MO(TEENSY_PINS);          // Length of the pin array
static const int delay = CONF_POLLING_DELAY;                  // Milliseconds between the time the teensy polls a chunk of pins
#undef X

// ALT: allocate all GPIO pins so index matches GPIO number
extern int getPinValue(const int GPIO_Pin);

extern void setPinValue(const int GPIO_Pin, const int value);

extern void update(void);

extern void initialize(void);

} // namespace Pins

#endif // __PINPOLLING_H__