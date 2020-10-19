#ifndef __PINHANDLERS_H__
#define __PINHANDLERS_H__

#include "WProgram.h"

namespace PinHandle {

// Null handler for testing
extern inline void null(const int pin, int &value) {
}

extern inline void readDigital(const int pin, int &value) {
    value = digitalReadFast(pin);
}

extern inline void readAnalog(const int pin, int &value) {
    value = analogRead(pin);
}

extern inline void writeDigital(const int pin, int &value) {
    digitalWriteFast(pin, value);
}

extern inline void writeAnalog(const int pin, int &value) {
    analogWrite(pin, value);
}

} // namespace PinHandle

#endif // __PINHANDLERS_H__