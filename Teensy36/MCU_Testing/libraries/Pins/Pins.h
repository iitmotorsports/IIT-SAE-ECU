/**
 * @file Pins.h
 * @author IR
 * @brief Update, set, and get predefined pin values
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 * To define pin values refer to PinConfig.def
 * 
 */

#ifndef __MCU_PINS_H__
#define __MCU_PINS_H__

// IMPROVE: pin priority
// TODO: add analog resolution and frequency option

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Get and set values to predefined pins.
 * Refer to Pins.h for more info.
 */
namespace Pins {

/**
 * @brief A typedef for pin handler functions
 */
typedef void (*PinHandler)(const int pin, int &value);

/**
 * @brief Get the pin value of a predefined pin
 * 
 * @param GPIO_Pin The GPIO pin to get a value from
 * @return int Returns an int that represents either a digital or analog value
 */
int getPinValue(const int GPIO_Pin);
/**
 * @brief Set the pin value of a predefined pin
 * 
 * @param GPIO_Pin The GPIO pin to set
 * @param value The value to set the analog/digital pin to
 */
void setPinValue(const int GPIO_Pin, const int value);

/**
 * @brief Poll all pin values
 */
void update(void);

/**
 * @brief Initialize all predefined pins
 */
void initialize(void);

} // namespace Pins

#endif // __MCU_PINS_H__