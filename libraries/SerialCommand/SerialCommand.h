/**
 * @file SerialCommand.h
 * @author IR
 * @brief SerialCommand functionality
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

/**
 * @brief A simple way to run commands over serial
 */
namespace Command {

/**
 * @brief attach a single callback to a byte value that will be received over serial
 * 
 * @param command The byte integer
 * @param callback The callback
 */
void setCommand(uint8_t command, CommandCallback callback);

/**
 * @brief receive any command from serial by matching incoming bytes to a callback
 * 
 * @return int The byte received
 */
int receiveCommand(void);

} // namespace Command

#endif // __ECU_SERIALCOMMAND_H__