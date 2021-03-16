/**
 * @file ECUGlobalConfig.h
 * @author IR
 * @brief Configure global build properties
 * @version 0.1
 * @date 2021-02-16
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ECU_GLOBALCONFIG_H__
// @cond
#define __ECU_GLOBALCONFIG_H__
// @endcond

/**
 * @brief Define to enable normal logging on back ECU and checks to be run throughout libaries
 */
#define CONF_ECU_DEBUG 1

/**
 * @brief Front ECU value
 */
#define FRONT_ECU 1
/**
 * @brief Back ECU value
 */
#define BACK_ECU 0

// TODO: build script not ignoring python script when compiling secondary build

/**
 * @brief Defines build is for back ECU
 * @details Used for documentation
 */
#define CONF_ECU_POSITION BACK_ECU
// #define CONF_ECU_POSITION FRONT_ECU // Defines build is for front ECU

/**
 * @brief Set serial baud rate
 */
#define CONF_TEENSY_BAUD_RATE 115200

/**
 * @brief Set a delay on startup before a Teensy does anything
 */
#define CONF_TEENSY_INITAL_DELAY 2000

#endif // __ECU_GLOBALCONFIG_H__