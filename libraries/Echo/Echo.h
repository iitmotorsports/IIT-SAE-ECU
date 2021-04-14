/**
 * @file Echo.h
 * @author IR
 * @brief Echo can messages with a delay
 * @version 0.1
 * @date 2021-04-14
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ECU_ECHO_H__
#define __ECU_ECHO_H__

#include "stdint.h"
#include "stdlib.h"

namespace Echo {

/**
 * @brief 
 * 
 * @param delay The delay to echo the message back
 * @param address The outgoing address
 * @param buf The buffer array to be echoed
 */
void echo(uint32_t delay, const uint32_t address, uint8_t buf[8]);

/**
 * @brief 
 * 
 * @param delay The delay to echo the message back
 * @param address The outgoing address
 * @param buf_0 byte 0 of the buffer
 * @param buf_1 byte 1 of the buffer
 * @param buf_2 byte 2 of the buffer
 * @param buf_3 byte 3 of the buffer
 * @param buf_4 byte 4 of the buffer
 * @param buf_5 byte 5 of the buffer
 * @param buf_6 byte 6 of the buffer
 * @param buf_7 byte 7 of the buffer
 */
void echo(uint32_t delay, const uint32_t address, const uint8_t buf_0 = 0, const uint8_t buf_1 = 0, const uint8_t buf_2 = 0, const uint8_t buf_3 = 0, const uint8_t buf_4 = 0, const uint8_t buf_5 = 0, const uint8_t buf_6 = 0, const uint8_t buf_7 = 0);

/**
 * @brief Initialize on receiving ECU to echo messages
 * 
 */
void setup();

} // namespace Echo

#endif // __ECU_ECHO_H__