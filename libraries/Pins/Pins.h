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

#ifndef __ECU_PINS_H__
#define __ECU_PINS_H__

// IMPROVE: pin priority
// TODO: add analog resolution and frequency option, low priority

#include "PinConfig.def"
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Get and set values to predefined pins.
 * 
 * Manage defined pins and ensure only defined pins are accessed
 * 
 * Refer to Pins.h for more info.
 */
namespace Pins {

/**
 * @brief A typedef for pin handler functions
 */
typedef void (*PinHandler)(uint8_t CAN_GPIO_Pin, int &value);

/**
 * @brief Get the pin value of a predefined canbus pin
 * 
 * @note Pins must first be defined in PinConfig.def
 * 
 * @param CAN_GPIO_Pin The canbus GPIO pin to get a value from
 * @return int Returns an int that represents either a digital or analog value
 */
int getCanPinValue(uint8_t CAN_GPIO_Pin);

/**
 * @brief Get the pin value of a predefined pin
 * 
 * @param GPIO_Pin The GPIO pin to get a value from
 * @return int Returns an int that represents either a digital or analog value
 */
int getPinValue(uint8_t GPIO_Pin);

/**
 * @brief Set the pin value of a predefined pin
 * 
 * @param GPIO_Pin The GPIO pin to set
 * @param value The value to set the analog/digital pin to
 */
void setPinValue(uint8_t GPIO_Pin, int value);

/**
 * @brief Poll analog pin values
 */
void update(void);

/**
 * @brief Initialize all predefined pins
 */
void initialize(void);

} // namespace Pins

#endif // __ECU_PINS_H__