/**
 * @file SerialCommand.h
 * @author IR
 * @brief Serial read functionality
 * @version 0.1
 * @date 2021-03-16
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ECU_SERIALCOMMAND_H__
#define __ECU_SERIALCOMMAND_H__

#include "usb_serial.h"
#include <stdint.h>
#include <stdlib.h>

#define COMMAND_ENABLE_CHARGING 186
#define COMMAND_SEND_CANBUS_MESSAGE 218
#define COMMAND_CLEAR_FAULTS 379
#define COMMAND_TOGGLE_CANBUS_SNIFF 492
#define COMMAND_PING 518

typedef void (*CommandCallback)(uint8_t);

namespace Command {

void setCommand(uint8_t command, CommandCallback);

size_t readBuffer(uint8_t count, uint8_t buffer[8]) {
    return Serial.readBytes((char *)buffer, 8);
}

} // namespace Command

#endif // __ECU_SERIALCOMMAND_H__