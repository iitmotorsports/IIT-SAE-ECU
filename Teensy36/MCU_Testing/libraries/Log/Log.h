#ifndef __MCU_LOGGING_H__
#define __MCU_LOGGING_H__

#include <stdint.h>
#include <stdlib.h>

// Use for logging things
struct Log_t {
    // Log something to usb serial
    void operator()(const char *TAG, const char *format, ...);
    // Log using a debug tag
    void d(const char *TAG, const char *format, ...);
    // Log using an info tag
    void i(const char *TAG, const char *format, ...);
    // Log using a warning tag
    void w(const char *TAG, const char *format, ...);
    // Log using an error tag
    void e(const char *TAG, const char *format, ...);
    // Log using a fatal tag
    void f(const char *TAG, const char *format, ...);
};

static Log_t Log;

#endif // __MCU_LOGGING_H__