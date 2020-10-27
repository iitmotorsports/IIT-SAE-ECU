#ifndef __MCU_PINS_H__
#define __MCU_PINS_H__

// IMPROVE: pin priority
// TODO: add analog resolution and frequency option

#include <stdint.h>
#include <stdlib.h>

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

int getPinValue(const int GPIO_Pin);
void setPinValue(const int GPIO_Pin, const int value);
void update(void);
void initialize(void);

} // namespace Pins

#endif // __MCU_PINS_H__