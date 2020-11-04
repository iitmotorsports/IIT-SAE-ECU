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

#if CONF_LOGGING_MAPPED_MODE > 0

#define LOG_END_MSG_FLAG CONF_LOGGING_END_MSG_FLAG

// No Timestamping for this mode

static void __logger_print(const char *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE) {
    Serial.write((const uint8_t *)&TAG, 2);
    uint64_t buffer = (MESSAGE & 0x0000FFFF);
    buffer = buffer << 32;
    Serial.write((const uint8_t *)&buffer, 8);
    Serial.write(LOG_END_MSG_FLAG);
}

static void __logger_print(const char *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE, const uint32_t NUMBER) {
    Serial.write((const uint8_t *)&TAG, 2);
    uint64_t buffer = (MESSAGE & 0x0000FFFF);
    buffer = buffer << 32 | NUMBER;
    Serial.write((const uint8_t *)&buffer, 8);
    Serial.write(LOG_END_MSG_FLAG);
}

#else

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
static const char *FORMAT = "%s @ %u [%s]: %s\n";
static const char *FORMAT_NUM = "%s @ %u [%s]: %s %u\n";
static void __logger_print(const char *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE) { Serial.printf(FORMAT, TYPE, millis(), TAG, MESSAGE); }
static void __logger_print_num(const char *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE, const uint32_t NUMBER) { Serial.printf(FORMAT_NUM, TYPE, millis(), TAG, MESSAGE, NUMBER); }
#else
static const char *FORMAT = "%s [%s]: %s\n";
static const char *FORMAT_NUM = "%s [%s]: %s %u\n";
static void __logger_print(const char *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE) { Serial.printf(FORMAT, TYPE, TAG, MESSAGE); }
static void __logger_print_num(const char *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE, const uint32_t NUMBER) { Serial.printf(FORMAT_NUM, TYPE, TAG, MESSAGE, NUMBER); }
#endif
#endif

#endif

void Log_t::operator()(LOG_TAG TAG, LOG_MSG message) {
#ifdef __LOGGER_NONE_PRINT
    __logger_print(NONE, TAG, message);
#endif
}

void Log_t::d(LOG_TAG TAG, LOG_MSG message) {
#ifdef __LOGGER_DEBUG_PRINT
    __logger_print(DEBUG, TAG, message);
#endif
}

void Log_t::i(LOG_TAG TAG, LOG_MSG message) {
#ifdef __LOGGER_INFO_PRINT
    __logger_print(INFO, TAG, message);
#endif
}

void Log_t::w(LOG_TAG TAG, LOG_MSG message) {
#ifdef __LOGGER_WARN_PRINT
    __logger_print(WARN, TAG, message);
#endif
}

void Log_t::e(LOG_TAG TAG, LOG_MSG message) {
#ifdef __LOGGER_ERROR_PRINT
    __logger_print(ERROR, TAG, message);
#endif
}

void Log_t::f(LOG_TAG TAG, LOG_MSG message) {
#ifdef __LOGGER_FATAL_PRINT
    __logger_print(FATAL, TAG, message);
#endif
}

void Log_t::operator()(LOG_TAG TAG, LOG_MSG message, const uint32_t number) {
#ifdef __LOGGER_NONE_PRINT
    __logger_print_num(NONE, TAG, message, number);
#endif
}

void Log_t::d(LOG_TAG TAG, LOG_MSG message, const uint32_t number) {
#ifdef __LOGGER_DEBUG_PRINT
    __logger_print_num(DEBUG, TAG, message, number);
#endif
}

void Log_t::i(LOG_TAG TAG, LOG_MSG message, const uint32_t number) {
#ifdef __LOGGER_INFO_PRINT
    __logger_print_num(INFO, TAG, message, number);
#endif
}

void Log_t::w(LOG_TAG TAG, LOG_MSG message, const uint32_t number) {
#ifdef __LOGGER_WARN_PRINT
    __logger_print_num(WARN, TAG, message, number);
#endif
}

void Log_t::e(LOG_TAG TAG, LOG_MSG message, const uint32_t number) {
#ifdef __LOGGER_ERROR_PRINT
    __logger_print_num(ERROR, TAG, message, number);
#endif
}

void Log_t::f(LOG_TAG TAG, LOG_MSG message, const uint32_t number) {
#ifdef __LOGGER_FATAL_PRINT
    __logger_print_num(FATAL, TAG, message, number);
#endif
}

Log_t Log;