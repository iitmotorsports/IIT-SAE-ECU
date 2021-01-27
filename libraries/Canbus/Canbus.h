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
 * @brief Update the Canbus line
 * This update function will check the rx buffer for any messages and update values.
 * Must be run periodically when using canbus
 */
void update(void);

/**
 * @brief Setup the teensy Canbus line
 */
void setup(void);

/**
 * @brief Get raw data from a canbus address
 * 
 * @note Only valid incoming addresses will put data onto the given buffer
 * 
 * @param address The incoming address
 * @param buf the buffer to copy data to
 */
void getData(const uint32_t address, uint8_t buf[8]);

/**
 * @brief Get the buffer of an outgoing address in order to set it's values to be pushed later.
 * Use pushData to push the data after modifying the buffer.
 * Invalid addresses will return a buffer that is ignored.
 * @param address The outgoing address
 * @return uint8_t[8] buffer array of the message, length 8
 */
uint8_t *getBuffer(const uint32_t address);

/**
 * @brief queue and address's buffer to be pushed.
 * Invalid addresses will not do anything.
 * 
 * @param address The outgoing address
 */
void pushData(const uint32_t address);

/**
 * @brief Send raw data over a given canbus address using a given array
 * 
 * @note Function does not verify that address is outgoing, undefined behavior will occur if data is sent thorugh an incoming address
 * 
 * @param address The outgoing address
 */
void sendData(const uint32_t address, uint8_t buf[8]);

/**
 * @brief Send raw data over a given canbus address using given values
 * 
 * @note Function does not verify that address is outgoing, undefined behavior will occur if data is sent thorugh an incoming address
 * 
 * @param address The outgoing address
 * @param buf_0 byte 0 of the outgoing buffer
 * @param buf_1 byte 1 of the outgoing buffer
 * @param buf_2 byte 2 of the outgoing buffer
 * @param buf_3 byte 3 of the outgoing buffer
 * @param buf_4 byte 4 of the outgoing buffer
 * @param buf_5 byte 5 of the outgoing buffer
 * @param buf_6 byte 6 of the outgoing buffer
 * @param buf_7 byte 7 of the outgoing buffer
 */
void sendData(const uint32_t address, const uint8_t buf_0 = 0, const uint8_t buf_1 = 0, const uint8_t buf_2 = 0, const uint8_t buf_3 = 0, const uint8_t buf_4 = 0, const uint8_t buf_5 = 0, const uint8_t buf_6 = 0, const uint8_t buf_7 = 0);

} // namespace Canbus

#endif // __ECU_CANBUS_H__