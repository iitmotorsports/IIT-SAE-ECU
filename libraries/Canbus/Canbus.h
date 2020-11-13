/**
 * @file Canbus.h
 * @author IR
 * @brief FlexCAN_T4 wrapper
 * This library is made specifically for SAE
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __ECU_CANBUS_H__
#define __ECU_CANBUS_H__

#include <stdint.h>
#include <stdlib.h>

#include "CanbusConfig.def"
#include "FlexCAN_T4.h"
#include "WProgram.h"

/**
 * @brief Canbus functionality.
 * Refer to Canbus.h for more info.
 */
namespace Canbus {

/**
 * @brief Setup the teensy Canbus line
 */
void setup(void);
/**
 * @brief Update the Canbus line
 * This update function will check the canbus for any messages and run the appropriate handler
 */
void update(void);

} // namespace Canbus

#endif // __ECU_CANBUS_H__