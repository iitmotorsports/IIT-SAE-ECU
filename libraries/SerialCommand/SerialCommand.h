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

#include "SerialCommand.def"
#include "usb_serial.h"
#include <stdint.h>
#include <stdlib.h>

typedef void (*CommandCallback)(void);

namespace Command {

void setCommand(uint8_t command, CommandCallback callback);

void receiveCommand();

} // namespace Command

#endif // __ECU_SERIALCOMMAND_H__