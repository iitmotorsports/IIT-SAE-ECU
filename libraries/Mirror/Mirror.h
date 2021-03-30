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