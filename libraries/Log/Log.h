/**
 * @file Log.h
 * @author IR
 * @brief Special logging functionality
 * @version 0.1
 * @date 2020-11-12
 * 
 * @copyright Copyright (c) 2020
 * 
 * @see Logging::Log_t for info on how logging works
 * @see LogConfig.def for configuration of logging
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

// IMPROVE: Add option to log to an sd card instead/as well

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
 * @brief Return the final numbervalue of a LOG_TAG
 * @warning If mapped mode is not on, this will just return 0
 * 
 * @param tagValue The LOG_TAG to convert
 * @return uint16_t the number representation of the tag
 */
uint32_t TAG2NUM(LOG_TAG tagValue);

/**
 * @brief Namespace to isolate Log_t struct.
 * @see Log.h for more info.
 */
namespace Logging {

/**
 * @brief Base class used to log things over serial
 * 
 * Logging is centered around communicating with the companion app.
 * 
 * ### Default Mode
 * 
 * In its default mode, logging does not send ASCII strings but integer numbers which represent a set of defined strings.
 * 
 * The actual messages that are sent over serial are as follows.
 * 
 * ```
 * Byte Number |0    |1   |2   |3   |4   |5   |6   |7   |
 * What it is  |State Code|String ID|      uint32_t     |
 * ```
 * 
 * Bytes 0-1 represent a State Code, or in other works, a unique number that is mapped to a unique string
 * representing the name of what is making this log entry.
 * 
 * Bytes 2-3 represent a String ID, a unique number that is mapped to a unique string.
 * 
 * Bytes 4-7 represent a 32 bit value, by default it is zero, every log entry has this number sent.
 * 
 * Calls to the global `Log` can be done as follows.
 * 
 * ``` C
 * Log(TAG, message, number);
 * ```
 * 
 * `TAG` can be staticly defined using a `LOG_TAG`.
 * 
 * `message` must be a string literal as such.
 * 
 * ``` C
 *      Log("String goes here"); ✔️
 *      Log(someStringReference); ❌
 * ```
 * 
 * Unless TAG2NUM() is used.
 * 
 * And `number` can be any unsigned 32 bit integer.
 * 
 * The companion app received this messages as an 8 byte integer and decodes them by first splitting them into
 * the structure shown, and then matching those number that Pre_Build.py generates in `log_lookup.json`
 * 
 * ### Debug ASCII Mode
 * 
 * When the prebuild script Pre_Build.py is skipped, calls to `Log` still function, however the message sent is
 * instead a formatted ASCII string, no string mapping or anything else occurs.
 * 
 * This mode also has the option to prepend a time stamp
 * 
 * ### Logging Levels
 * 
 * Logging has different levels which can be logged to, this is used to differentiate between how important different
 * log entries are.
 * 
 * The current types are as follows.
 * 
 * |Func Call   |Log Type       |
 * |------------|---------------|
 * |Log.d()     |Debug log      |
 * |Log.i()     |Info log       |
 * |Log()       |Normal log     |
 * |Log.w()     |Warning log    |
 * |Log.e()     |Error log      |
 * |Log.f()     |Fatal Log      |
 * 
 * ### Logging over Can
 * 
 * The front ECU can easily communicate with the companion app as it is connected directly with USB serial.
 * 
 * However, the back ECU is only connected to the front ECU through CanBus.
 * 
 * To enable logging on the back ECU, it has to send messages over Canbus.
 * This method is only supported when Logging is in it's default mode.
 * 
 * Running Logging::enableCanbusRelay() once on the front ECU ensure that it receives and relays messages from
 * the back ECU to the companion app.
 * 
 * @warning Because of the nature of serial and its interaction with interrupts, which is how messages are relayed,
 * it is recommended to instead use CanPins to communicate data from the back ECU to the front ECU.
 * 
 * @see Pins.h for more info on CanPins
 * @see Pre_Build.py for more info on how calls to Logging related items are modified before compilation
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

/**
 * @brief Print the ZLib compressed string of the current lookup table to serial
 * @note First prints 8 zero bytes, then the length of the array as a ulong, then the actual map.
 * @warning **A byte of serial data must be sent back after 8 bytes of the map is received, this is to help mediate the amount of data sent**
 */
void printLookup();

} // namespace Logging

/**
 * @brief The *global* logging object
 * 
 * @see Logging::Log_t for more info on logging
 */
extern Logging::Log_t Log;

#endif // __ECU_LOGGING_H__