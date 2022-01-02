/**
 * @file Log.cpp
 * @author IR
 * @brief Log.h source file
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */

// @cond

#include <cstring>
#include <stdarg.h>
#include <stdio.h>

#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "Log.h"
#include "LogConfig.def"
#include "WProgram.h"
#include "unordered_map"

/**
 * @brief The ZLib compressed lookup table
 */
extern unsigned char log_lookup[];

/**
 * @brief The length of log_lookup's data
 */
extern unsigned int log_lookup_len;

/**
 * @brief The length of log_lookup's data with padding
 */
extern unsigned int log_lookup_pad_len;

/**
 * @brief The length of log_lookup's data when uncompressed
 */
extern unsigned int log_lookup_uncmp_len;

namespace Logging {

#ifdef SILENT
#ifdef CONF_LOGGING_MAX_LEVEL
#undef CONF_LOGGING_MAX_LEVEL
#endif
#define CONF_LOGGING_MAX_LEVEL 0
#endif

#ifndef CONF_LOGGING_MAX_LEVEL
#define CONF_LOGGING_MAX_LEVEL 4
#endif

#if CONF_LOGGING_MAX_LEVEL >= 5
#define __LOGGER_NONE_PRINT
#define __LOGGER_DEBUG_PRINT
#define __LOGGER_INFO_PRINT
#define __LOGGER_WARN_PRINT
#define __LOGGER_ERROR_PRINT
#define __LOGGER_FATAL_PRINT
#elif CONF_LOGGING_MAX_LEVEL == 4
#define __LOGGER_NONE_PRINT
#define __LOGGER_INFO_PRINT
#define __LOGGER_WARN_PRINT
#define __LOGGER_ERROR_PRINT
#define __LOGGER_FATAL_PRINT
#elif CONF_LOGGING_MAX_LEVEL == 3
#define __LOGGER_NONE_PRINT
#define __LOGGER_WARN_PRINT
#define __LOGGER_ERROR_PRINT
#define __LOGGER_FATAL_PRINT
#elif CONF_LOGGING_MAX_LEVEL == 2
#define __LOGGER_WARN_PRINT
#define __LOGGER_ERROR_PRINT
#define __LOGGER_FATAL_PRINT
#elif CONF_LOGGING_MAX_LEVEL == 1
#define __LOGGER_ERROR_PRINT
#define __LOGGER_FATAL_PRINT
#endif

#if CONF_LOGGING_MAPPED_MODE > 0

struct uMsg {
    uint32_t LAST_NUMBER = 0;
    elapsedMillis last;
    bool update(uint32_t newNum, int mediate) {
        /**
         * Mediate Value
         * 0 :   Log instantly
         * 1 :   Log only when value changes
         * >1:   Log after set delay or when value changes
         * <0:   Log only if value has changed after set -delay
         */
        if ((mediate >= 1 && newNum != LAST_NUMBER) || (mediate > 1 && (int)last > mediate) || (mediate < 0 && ((int)last > -mediate && newNum != LAST_NUMBER))) {
            last = 0;
            LAST_NUMBER = newNum;
            return true;
        }
        return false;
    }
};

struct uTag {
    std::unordered_map<uint32_t, uMsg> uniqueMessages;
    bool update(uint32_t msg, uint32_t num, int mediate) {
        return uniqueMessages[msg].update(num, mediate);
    }
};

static std::unordered_map<uint16_t, uTag> uniqueTags;

// No Timestamping for this mode

// Do this to use the same fuction header, we don't need these for mapped logging
static void *NONE;
static void *DEBUG = NONE;
static void *INFO = NONE;
static void *WARN = NONE;
static void *ERROR = NONE;
static void *FATAL = NONE;

static uint8_t log_buf[8] = {0};

/**
 * |0    |1   |2   |3   |4   |5   |6   |7   |
 * |State Code|String ID|      uint32_t     |
 */
static void __logger_print_num(void *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE, const uint32_t NUMBER, int mediate) {
    if (!mediate || uniqueTags[TAG].update(MESSAGE, NUMBER, mediate)) {
        memcpy(log_buf, &TAG, 2);
        memcpy(log_buf + 2, &MESSAGE, 2);
        memcpy(log_buf + 4, &NUMBER, 4);
#if CONF_ECU_POSITION == BACK_ECU
#ifdef CONF_ECU_DEBUG
        Serial.write(log_buf, 8);
#endif
        Canbus::sendData(ADD_AUX_LOGGING, log_buf);
#else
        Serial.write(log_buf, 8);
#endif
    }
}

/**
 * |0    |1   |2   |3   |4   |5   |6   |7   |
 * |State Code|String ID|         0         |
 */
static void __logger_print(void *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE) {
    __logger_print_num(TYPE, TAG, MESSAGE, 0, 0);
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
static void __logger_print_num(const char *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE, const uint32_t NUMBER, int mediate = false) { Serial.printf(FORMAT_NUM, TYPE, millis(), TAG, MESSAGE, NUMBER); }
#else
static const char *FORMAT = "%s [%s]: %s\n";
static const char *FORMAT_NUM = "%s [%s]: %s %u\n";
static void __logger_print(const char *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE) { Serial.printf(FORMAT, TYPE, TAG, MESSAGE); }
static void __logger_print_num(const char *TYPE, LOG_TAG TAG, LOG_MSG MESSAGE, const uint32_t NUMBER, int mediate = false) { Serial.printf(FORMAT_NUM, TYPE, TAG, MESSAGE, NUMBER); }
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

void Log_t::operator()(LOG_TAG TAG, LOG_MSG message, const uint32_t number, int mediate) {
#ifdef __LOGGER_NONE_PRINT
    __logger_print_num(NONE, TAG, message, number, mediate);
#endif
}

void Log_t::d(LOG_TAG TAG, LOG_MSG message, const uint32_t number, int mediate) {
#ifdef __LOGGER_DEBUG_PRINT
    __logger_print_num(DEBUG, TAG, message, number, mediate);
#endif
}

void Log_t::i(LOG_TAG TAG, LOG_MSG message, const uint32_t number, int mediate) {
#ifdef __LOGGER_INFO_PRINT
    __logger_print_num(INFO, TAG, message, number, mediate);
#endif
}

void Log_t::w(LOG_TAG TAG, LOG_MSG message, const uint32_t number, int mediate) {
#ifdef __LOGGER_WARN_PRINT
    __logger_print_num(WARN, TAG, message, number, mediate);
#endif
}

void Log_t::e(LOG_TAG TAG, LOG_MSG message, const uint32_t number, int mediate) {
#ifdef __LOGGER_ERROR_PRINT
    __logger_print_num(ERROR, TAG, message, number, mediate);
#endif
}

void Log_t::f(LOG_TAG TAG, LOG_MSG message, const uint32_t number, int mediate) {
#ifdef __LOGGER_FATAL_PRINT
    __logger_print_num(FATAL, TAG, message, number, mediate);
#endif
}

static void _receiveLogBuffer(uint32_t address, volatile uint8_t *buf) {
    Serial.write((uint8_t *)buf, 8); // FIXME: does dropping the volatile cause any issues?
}

void enableCanbusRelay() {
    Canbus::addCallback(ADD_AUX_LOGGING, _receiveLogBuffer);
}

void printLookup() {
    static elapsedMillis timeElapsed;
    noInterrupts();
    Serial.flush();
    uint64_t ullen = 0;
    uint8_t *bytes = (uint8_t *)&(ullen);
    Serial.write(bytes, 8);
    ullen = log_lookup_len;
    Serial.write(bytes, 8);
    ullen = log_lookup_uncmp_len;
    Serial.write(bytes, 8);
    timeElapsed = 0;
    for (size_t i = 0; i < log_lookup_pad_len; i += 8) {
        Serial.write(log_lookup + i, 8);
        Serial.flush();
        while (!Serial.available()) {
            if (timeElapsed > 31000) { // Entire process should take less than 30s
                interrupts();
                return;
            }
        }
        Serial.read();
    }
    interrupts();
}

} // namespace Logging

/**
 * @brief Internal definition of static Log class
 */
Logging::Log_t Log;

#if CONF_LOGGING_MAPPED_MODE > 0
uint32_t TAG2NUM(LOG_TAG tagValue) {
    return tagValue;
}
#else
uint32_t TAG2NUM(LOG_TAG tagValue) {
    return 0;
}
#endif

// @endcond
