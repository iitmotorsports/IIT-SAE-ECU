/**
 * @file Pins.h
 * @author IR
 * @brief Update, set, and get predefined pin values
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 * This module is used to simplify the usage of an ECU's GPIO pins.
 * 
 * **Types**
 * 
 * Through this module, each pin is predefined to be of a certain type.
 * 
 * These types include
 * * Analog or Digital
 * * Input or Output
 * 
 * @note Pins with things like PWM or a DAC are used by default as the underlying gpio library does that.
 * 
 * **Canbus**
 * 
 * Every pin that is set as an input on one ECU can be received on another ECU over Canbus.
 * 
 * Sending pins over canbus is not limited to physical GPIO, virtual pins can also be created which are interacted with similarly to normal pins.
 * 
 * These pins, virtual or non virtual, that are sent over Canbus are referred to as CanPins.
 * 
 * **Usage**
 * 
 * Pins can either be read or set depending on whether they are input or output respectively.
 * 
 * Their actual values depend on whether they are pre-defined as an analog or digital pin, this is the same for CanPins.
 * 
 * Pins are normally interacted with by using Pins::getPinValue() and Pins::setPinValue().
 * 
 * CanPins can also be used through the same functions, however, it is recommend to explicitly call Pins::setInternalValue() and Pins::getCanPinValue()
 * instead, as I have not actually tested the previous, and its just good practice I think.
 * 
 * @see PinConfig.def for more info on defining pin/canpin values and configuration of this module
 * 
 */

#ifndef __ECU_PINS_H__
#define __ECU_PINS_H__

// IMPROVE: pin priority

#include <stdint.h>
#include <stdlib.h>

#include "PPHelp.h"
#include "PinConfig.def"

/**
 * @brief The current target bit resolution of the ECU, set by PinConfig.def
 */
#define PINS_ANALOG_RES CONF_PINS_ANALOG_WRITE_RESOLUTION
/**
 * @brief The maximum analog value, given the current PINS_ANALOG_RES
 */
#define PINS_ANALOG_MAX pwrtwo(PINS_ANALOG_RES)
/**
 * @brief The *high* analog value, will not force an analog output to lock onto a high state, as PINS_ANALOG_MAX does
 */
#define PINS_ANALOG_HIGH (PINS_ANALOG_MAX - 1)
/**
 * @brief The minimum analog value. Its zero, I think it will always be zero. Not sure why i needed this.
 */
#define PINS_ANALOG_MIN 0

/**
 * @brief Maps Voltages 0-5v to an appropriate analog value
 */
#define PINS_VOLT_TO_ANALOG(x) (int)min(max((x / 5) * PINS_ANALOG_HIGH, PINS_ANALOG_MIN), PINS_ANALOG_HIGH)

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
 * @brief Set the value of an internal pin
 * @details These are fake pins that are usefully for sending gpio like values over canbus, See PinConfig.def for more info
 * 
 * @param Internal_Pin The Internal pin to set
 * @param value The value to set the analog/digital pin to
 */
void setInternalValue(uint8_t Internal_Pin, int value);

/**
 * @brief Resets physical pins to their inital state
 * @details This function sets pins as input/output and, if an output pin, sets them to their given init value. This is defined in PinConfig.def
 * 
 */
void resetPhysicalPins();

/**
 * @brief Poll analog pin values
 * @note WIP
 */
void update(void);

/**
 * @brief Stops background interrupts from sending canPins
 */
void stopCanPins(void);

/**
 * @brief Starts background interrupts to send canPins, if any are to be sent
 */
void startCanPins(void);

/**
 * @brief Initialize all predefined pins
 */
void initialize(void);

/**
 * @brief Used for debugging
 */
void debugPrint(void);

} // namespace Pins

#endif // __ECU_PINS_H__