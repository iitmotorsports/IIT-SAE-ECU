/**
 * @file Echo.h
 * @author IR
 * @brief Echo can messages with a delay
 * @version 0.1
 * @date 2021-04-14
 * 
 * @copyright Copyright (c) 2021
 * 
 * @see Echo for more info.
 */

#ifndef __ECU_ECHO_H__
#define __ECU_ECHO_H__

#include "stdint.h"
#include "stdlib.h"

/**
 * @brief This Module can be used to echo a CAN message over a network of two ECUs
 * 
 * Echo::echo is called when a serial command from the companion app sends a valid canbus message to relay
 * 
 * This modules is only used for testing
 * 
 * @see SerialCommand.h for more info on how Echo functions are called over serial
 */
namespace Echo {

/**
 * @brief Send a can message to the back ECU from the front to then echo the same message at a set delay
 * 
 * @param delay The delay to echo the message back
 * @param address The outgoing address
 * @param buf The buffer array to be echoed
 */
void echo(uint32_t delay, const uint32_t address, uint8_t buf[8]);

/**
 * @brief Send a can message to the back ECU from the front to then echo the same message at a set delay
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
 */
void setup();

} // namespace Echo

#endif // __ECU_ECHO_H__