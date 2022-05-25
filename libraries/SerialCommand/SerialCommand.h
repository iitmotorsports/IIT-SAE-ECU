/**
 * @file SerialCommand.h
 * @author IR
 * @brief SerialCommand functionality
 * @version 0.1
 * @date 2021-03-16
 *
 * @copyright Copyright (c) 2022
 *
 * This module is used to run commands on an ECU by sending specific byte codes over serial.
 *
 * Commands can be defined by calling Cmd::setCommand(), where the `uint8_t command` argument is defined in SerialCommand.def
 *
 * Cmd::receiveCommand() must be called to poll Serial and check for any available commands
 *
 * @see SerialCommand.def to define new command numbers
 *
 */

#ifndef __ECU_SERIALCOMMAND_H__
#define __ECU_SERIALCOMMAND_H__l

#include "SerialCommand.def"
#include "core_pins.h"
#include "usb_serial.h"
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief A function that will be called whenever its corresponding command byte is received
 */
typedef void (*CommandCallback)(void);

/**
 * @brief A simple way to run commands over serial
 */
namespace Cmd {

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

} // namespace Cmd

#endif // __ECU_SERIALCOMMAND_H__