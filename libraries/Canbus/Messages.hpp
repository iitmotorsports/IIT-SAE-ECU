/**
 * @file Messages.hpp
 * @author IR
 * @brief Functions that are used for messages
 * @version 0.1
 * @date 2020-11-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __ECU_MESSAGES_HPP__
#define __ECU_MESSAGES_HPP__

#include "CanbusConfig.def"
#include "FlexCAN_T4.h"
#include "WProgram.h"

/**
 * @brief Message handlers to define incoming mailboxes with
 * Refer to CanBusMessages.def
 * 
 */
namespace CanbusMsg {

/**
 * @brief Print a message to serial
 * 
 * @param msg 
 */
void SerialPrintMessage(const CAN_message_t &msg);

/**
 * @brief Check message for a BMS Fault
 * 
 * @param msg FlexCAN message
 */
void BMSFaultMsg(const CAN_message_t &msg);

/**
 * @brief Check message for a RMS Fault
 * 
 * @param msg FlexCAN message
 */
void RMSFaultMsg(const CAN_message_t &msg);

} // namespace CanbusMsg

#endif // __ECU_MESSAGES_HPP__