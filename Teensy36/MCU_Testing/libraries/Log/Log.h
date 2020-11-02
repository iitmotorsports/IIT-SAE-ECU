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
typedef const char *LOG_TAG;
typedef const char *LOG_MSG;
#endif

// Use for logging things
struct Log_t {
    // Log something to usb serial
    void operator()(LOG_TAG TAG, LOG_MSG message);
    // Log using a debug tag
    void d(LOG_TAG TAG, LOG_MSG message);
    // Log using an info tag
    void i(LOG_TAG TAG, LOG_MSG message);
    // Log using a warning tag
    void w(LOG_TAG TAG, LOG_MSG message);
    // Log using an error tag
    void e(LOG_TAG TAG, LOG_MSG message);
    // Log using a fatal tag
    void f(LOG_TAG TAG, LOG_MSG message);
    // Log something to usb serial with a variable number
    void operator()(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    // Log using a debug tag with a variable number
    void d(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    // Log using an info tag with a variable number
    void i(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    // Log using a warning tag with a variable number
    void w(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    // Log using an error tag with a variable number
    void e(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
    // Log using a fatal tag with a variable number
    void f(LOG_TAG TAG, LOG_MSG message, const uint32_t number);
};

extern Log_t Log;

#endif // __MCU_LOGGING_H__