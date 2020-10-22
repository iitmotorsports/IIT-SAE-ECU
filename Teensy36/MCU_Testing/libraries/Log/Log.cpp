/**
 * Manage all that there is to Notifying the tablet about what is happening
 * Not for sending raw_data, low priority logging library
 * Potential for logging to sd card
 */

#include <stdarg.h>
#include <stdio.h>

#include "Log.h"
#include "WProgram.h"

static const char *NONE = "[  LOG  ]";
static const char *DEBUG = "[ DEBUG ]";
static const char *INFO = "[ INFO  ]";
static const char *WARN = "[ WARN  ]";
static const char *ERROR = "[ ERROR ]";
static const char *FATAL = "[ FATAL ]";
static const char *FORMAT = "%s [%s]   \t";

static void print(const char *format, __va_list args) {
    vdprintf((int)&Serial, format, args);
    Serial.write('\n');
}

void Log_t::operator()(const char *TAG, const char *format, ...) {
    Serial.printf(FORMAT, NONE, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
}

void Log_t::d(const char *TAG, const char *format, ...) {
    Serial.printf(FORMAT, DEBUG, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
}

void Log_t::i(const char *TAG, const char *format, ...) {
    Serial.printf(FORMAT, INFO, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
}

void Log_t::w(const char *TAG, const char *format, ...) {
    Serial.printf(FORMAT, WARN, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
}

void Log_t::e(const char *TAG, const char *format, ...) {
    Serial.printf(FORMAT, ERROR, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
}

void Log_t::f(const char *TAG, const char *format, ...) {
    Serial.printf(FORMAT, FATAL, TAG);
    va_list args;
    va_start(args, format);
    print(format, args);
}