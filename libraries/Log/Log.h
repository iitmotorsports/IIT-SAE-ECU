/**
 * @file Log.h
 * @author IR
 * @brief Special logging functionality
 * @version 0.1
 * @date 2020-11-12
 * 
 * @copyright Copyright (c) 2020
 * 
 * To modify how this library behaves, refer to the LogConfig.def
 * 
 */
#ifndef __ECU_LOGGING_H__
#define __ECU_LOGGING_H__

/**
 * @brief Refer to Pre_Build.py
 */
#define _LogPrebuildString(x) x

#include "LogConfig.def"
#include <stdint.h>
#include <stdlib.h>

// IMPROVE: Add option to log to an sd card instead/aswell

#if CONF_LOGGING_MAPPED_MODE > 0
typedef const uint16_t LOG_TAG;
typedef const uint32_t LOG_MSG;
#else
/**
 * @brief Type definition of logging tags
 * This typedef is necessary to allow for easier manipulation of code by Pre_Build.py
 * @note If Pre_Build.py is run, **All** declerations of `LOG_TAG` must be an inline string \n
 * 
 * Example usage ( Note that LOG_TAG_t should actually be LOG_TAG ) :
 *      LOG_TAG_t TAG = "Logging Tag"; ✔️ \n
 *      LOG_TAG_t TAG = someTagReference; ❌
 */
typedef const char *LOG_TAG;
/**
 * @brief Type definition for logging messages, only used internally
 */
typedef const char *LOG_MSG;
#endif

/**
 * @brief Namespace to isolate Log_t struct.
 * Refer to Log.h for more info.
 */
namespace Logging {

/**
 * @brief Base class used to log things to serial \n
 * This class has the capability to modify all of it's functions to instead use integer IDs \n
 * Refer to the Pre_Build.py script for more information
 * @note If Pre_Build.py is run, **All** strings passed to parameter `message` must be an inline string \n
 *      Log("String goes here"); ✔️ \n
 *      Log(someStringReference); ❌
 */
struct Log_t {
    /**
     * @brief Log a string usb serial
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     */
    void operator()(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string using a debug tag
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     */
    void d(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string using an info tag
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     */
    void i(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string using a warning tag
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     */
    void w(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string using an error tag
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     */
    void e(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string using a fatal tag
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     */
    void f(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string and a variable number to usb serial
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void operator()(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    /**
     * @brief Log a string and a variable number using a debug tag
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void d(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    /**
     * @brief Log a string and a variable number using an info tag
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void i(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    /**
     * @brief Log a string and a variable number using a warning tag
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void w(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    /**
     * @brief Log a string and a variable number using an error tag
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void e(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    /**
     * @brief Log a string and a variable number using a fatal tag
     * 
     * @param TAG Variable of type `LOG_TAG`
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void f(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
};

/**
 * @brief If a set address is received through canbus, the data will be pushed to a buffer to be printed
 * @note The address is set in CanBusAddresses.def
 */
void enableCanbusRelay();

} // namespace Logging

/**
 * @brief Refer to Log_t
 */
extern Logging::Log_t Log;

#endif // __ECU_LOGGING_H__