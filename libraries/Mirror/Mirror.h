/**
 * @file Mirror.h
 * @author IR
 * @brief This library allows for the monitoring and modification of each GPIO pin on an ECU
 * @version 0.1
 * @date 2021-03-30
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ECU_MIRROR_H__
#define __ECU_MIRROR_H__

/**
 * @brief Check and modify pin values over USB serial
 * How to use - connect with a terminal that can both send and receive values
 * compile and upload a binary with both debug and ascii mode enabled
 * 
 * Example Term https://www.der-hammer.info/pages/terminal.html
 *
 * First send the decimal value '90' or whatever is defined in SerialCommand.def
 * to enter Mirror mode.
 *
 * Commands (all in decimal):
 * byte != 255: print out this pin's value
 *      Sending a single number that is not 255 will return that corresponding pin's
 *      value if it exists
 *
 * byte == 255 : Enter set pin mode
 *      Wait for subsequent bytes to set a pin value.
 *      The next byte received is the pin to set.
 *      The next four bytes (little endian) is the integer to set that pin to.
 *      @note If not all four bytes are received then those bytes are zeroed
 *      
 *
 */
namespace Mirror {

/**
 * @brief Setup a listener for going into mirror mode, dependent on which ECU is compiled
 * 
 */
void setup(void);

/**
 * @brief Manually enter mirror mode, only returning when a set command is received
 * 
 */
void enterMirrorMode(void);

/**
 * @brief Manually exit mirror mode
 * @details Should be called by an external interrupt
 */
void exitMirrorMode(void);

} // namespace Mirror

#endif // __ECU_MIRROR_H__