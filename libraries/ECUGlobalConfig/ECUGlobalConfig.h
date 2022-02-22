/**
 * @file ECUGlobalConfig.h
 * @author IR
 * @brief Configure global build properties
 * @version 0.1
 * @date 2021-02-16
 * 
 * @copyright Copyright (c) 2022
 * 
 * This configuration file is used to define values used through out the entire project.
 * 
 */

#ifndef __ECU_GLOBALCONFIG_H__
// @cond
#define __ECU_GLOBALCONFIG_H__
// @endcond

/**
 * @brief Front ECU value
 */
#define FRONT_ECU 1
/**
 * @brief Back ECU value
 */
#define BACK_ECU 0

/**
 * @brief Define to enable normal logging on back ECU and checks to be run throughout libaries
 */
#define CONF_ECU_DEBUG 1

/**
 * @brief Options that should only be allowed if debugging
 */
#ifdef CONF_ECU_DEBUG

/**
 * @brief Define to disable logging, rendering an ECU 'silent'
 */
// #define SILENT

/**
 * @brief Define as either 'FRONT_ECU' or 'BACK_ECU' to enable testing mode on that ECU
 */
// #define ECU_TESTING FRONT_ECU

#endif

#ifndef CONF_ECU_POSITION
/**
 * @brief Defines build is for back ECU
 * @details Used for documentation
 */
#define CONF_ECU_POSITION BACK_ECU
// #define CONF_ECU_POSITION FRONT_ECU // Defines build is for front ECU
#endif

/**
 * @brief Set serial baud rate
 */
#define CONF_ECU_BAUD_RATE 115200

/**
 * @brief Set a delay on startup before any ECU does anything
 */
#define CONF_ECU_INITAL_DELAY 2000

#endif // __ECU_GLOBALCONFIG_H__