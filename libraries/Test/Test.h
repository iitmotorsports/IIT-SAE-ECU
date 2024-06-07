
/**
 * @file test.h
 * @author IR
 * @brief Basic functions used for testing
 * @version 0.1
 * @date 2021-12-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __ECU_TEST_H__
#define __ECU_TEST_H__

#include "ECUGlobalConfig.h"

/**
 * @brief Output fake data as if the front ECU were actively running
 */
void full_front_test();

/**
 * @brief Spam usb serial
 */
void serial_spam();

#endif // __ECU_TEST_H__