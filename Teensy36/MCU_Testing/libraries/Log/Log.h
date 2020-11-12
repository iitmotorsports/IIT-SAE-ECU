#ifndef __MCU_LOGGING_H__
#define __MCU_LOGGING_H__

#include "LogConfig.def"
#include <stdint.h>
#include <stdlib.h>

/** IMPROVE: create precompile script that converts any call to Log to instead send a two byte ID that matches a string corresponding to it's unique string
 *  in a generated lookup table that is output next to the compiled hex file
*/

#if CONF_LOGGING_MAPPED_MODE > 0
typedef const uint16_t LOG_TAG;
typedef const uint32_t LOG_MSG;
#else
/**
 * @brief Type definition of Logging tags
 * This typedef is necessary to allow for easier manipulation of code by Pre_Build.py
 * @note If Pre_Build.py is run, _All_ declerations of LOG_TAG must be an inline string \n
 *      LOG_TAG TAG = "Logging Tag"; ✔️ \n
 *      LOG_TAG TAG = someTagReference; ❌
 */
typedef const char *LOG_TAG;
typedef const char *LOG_MSG;
#endif

/**
 * @brief Base class used to log things to serial \n
 * This class has the capability to modify all of it's functions to instead use integer IDs \n
 * Refer to the Pre_Build.py script for more information
 * @note If Pre_Build.py is run, _All_ strings passed to parameter message must be an inline string \n
 *      Log("String goes here"); ✔️ \n
 *      Log(someStringReference); ❌
 */
struct Log_t {
    /**
     * @brief Log a string usb serial
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     */
    void operator()(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string using a debug tag
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     */
    void d(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string using an info tag
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     */
    void i(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string using a warning tag
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     */
    void w(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string using an error tag
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     */
    void e(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string using a fatal tag
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     */
    void f(LOG_TAG TAG, LOG_MSG message);
    /**
     * @brief Log a string and a variable number to usb serial
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void operator()(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    /**
     * @brief Log a string and a variable number using a debug tag
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void d(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    /**
     * @brief Log a string and a variable number using an info tag
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void i(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    /**
     * @brief Log a string and a variable number using a warning tag
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void w(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    /**
     * @brief Log a string and a variable number using an error tag
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void e(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    /**
     * @brief Log a string and a variable number using a fatal tag
     * 
     * @param TAG Variable of type LOG_TAG
     * @param message Inline string that should be printed
     * @param number Any number that should be printed next to the string
     */
    void f(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
};

/**
 * @brief Refer to Log_t
 */
extern Log_t Log;

// extern Log_t Log;

#endif // __MCU_LOGGING_H__