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

/**
 * @brief Enable mailbox interrupts, allowing any message handlers to run
 * 
 * @param enable boolean
 */
void enableInterrupts(bool enable);

/**
 * @brief Get the message from a mailbox if there is any
 * 
 * @note Interrupts must be disabled or no message will be returned
 * @note Do not modify the mailbox that is returned
 * 
 * @param MB Mailbox number
 * @return CAN_message_t returns a FlexCAN message, message will be completely empty if nothing was received
 */
CAN_message_t getMailbox(FLEXCAN_MAILBOX MB);

/**
 * @brief Set the handle function, will be called whenever any message is received
 * Use Canbus::getMailbox if only a specifc message is needed
 * 
 * @param handler Message handle accepts a `const CANFD_message_t &msg` and returns `void`
 */
void setHandle(_MB_ptr handler);

/**
 * @brief removes any handle function if there is one
 */
void clearHandle(void);

} // namespace Canbus

#endif // __ECU_CANBUS_H__