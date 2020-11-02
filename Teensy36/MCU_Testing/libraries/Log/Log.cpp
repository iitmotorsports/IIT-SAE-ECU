/**
 * Manage all that there is to Notifying the tablet about what is happening
 * Not for sending raw_data, low priority logging library
 * Potential for logging to sd card
 */

#include <stdarg.h>
#include <stdio.h>

#include "Log.h"
#include "LogConfig.def"
#include "WProgram.h"

#ifndef CONF_LOGGING_MAX_LEVEL
#define CONF_LOGGING_MAX_LEVEL 5
#endif

#if CONF_LOGGING_MAX_LEVEL >= 6
#define __LOGGER_NONE_PRINT
#define __LOGGER_DEBUG_PRINT
#define __LOGGER_INFO_PRINT
#define __LOGGER_WARN_PRINT
#define __LOGGER_ERROR_PRINT
#define __LOGGER_FATAL_PRINT
#elif CONF_LOGGING_MAX_LEVEL == 5
#define __LOGGER_NONE_PRINT
#define __LOGGER_INFO_PRINT
#define __LOGGER_WARN_PRINT
#define __LOGGER_ERROR_PRINT
#define __LOGGER_FATAL_PRINT
#elif CONF_LOGGING_MAX_LEVEL == 4
#define __LOGGER_NONE_PRINT
#define __LOGGER_WARN_PRINT
#define __LOGGER_ERROR_PRINT
#define __LOGGER_FATAL_PRINT
#elif CONF_LOGGING_MAX_LEVEL == 3
#define __LOGGER_NONE_PRINT
#define __LOGGER_ERROR_PRINT
#define __LOGGER_FATAL_PRINT
#elif CONF_LOGGING_MAX_LEVEL == 2
#define __LOGGER_NONE_PRINT
#define __LOGGER_FATAL_PRINT
#elif CONF_LOGGING_MAX_LEVEL == 1
#define __LOGGER_NONE_PRINT
#endif

#ifdef __LOGGER_NONE_PRINT
static const char *NONE = "[ LOG ]";
#ifdef __LOGGER_DEBUG_PRINT
static const char *DEBUG = "[DEBUG]";
#endif
#ifdef __LOGGER_INFO_PRINT
static const char *INFO = "[INFO] ";
#endif
#ifdef __LOGGER_WARN_PRINT
static const char *WARN = "[WARN] ";
#endif
#ifdef __LOGGER_ERROR_PRINT
static const char *ERROR = "[ERROR]";
#endif
#ifdef __LOGGER_FATAL_PRINT
static const char *FATAL = "[FATAL]";
#endif

#ifdef CONF_LOGGING_ENABLE_TIMESTAMP
static const char *FORMAT = "%s @ %u [%s]: ";
#define __logger_printf(FORMAT, TYPE, TAG) Serial.printf(FORMAT, TYPE, millis(), TAG);
#else
static const char *FORMAT = "%s [%s]: \t ";
#define __logger_printf(FORMAT, TYPE, TAG) Serial.printf(FORMAT, TYPE, TAG);
#endif

static void print(const char *format, va_list args) {
    vdprintf((int)&Serial, format, args);
    Serial.write('\n');
}

#endif

void Log_t::operator()(LOG_TAG TAG, const char *format, ...) {
#ifdef __LOGGER_NONE_PRINT
    __logger_printf(FORMAT, NONE, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
#endif
}

void Log_t::d(LOG_TAG TAG, const char *format, ...) {
#ifdef __LOGGER_DEBUG_PRINT
    __logger_printf(FORMAT, DEBUG, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
#endif
}

void Log_t::i(LOG_TAG TAG, const char *format, ...) {
#ifdef __LOGGER_INFO_PRINT
    __logger_printf(FORMAT, INFO, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
#endif
}

void Log_t::w(LOG_TAG TAG, const char *format, ...) {
#ifdef __LOGGER_WARN_PRINT
    __logger_printf(FORMAT, WARN, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
#endif
}

void Log_t::e(LOG_TAG TAG, const char *format, ...) {
#ifdef __LOGGER_ERROR_PRINT
    __logger_printf(FORMAT, ERROR, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
#endif
}

void Log_t::f(LOG_TAG TAG, const char *format, ...) {
#ifdef __LOGGER_FATAL_PRINT
    __logger_printf(FORMAT, FATAL, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
#endif
}

Log_t Log;