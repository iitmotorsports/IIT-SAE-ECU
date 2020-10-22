#include "Handlers.hpp"
#include "WProgram.h"

extern void PinHandlers::null(const int pin, int &value) {
}

extern void PinHandlers::readDigital(const int pin, int &value) {
    value = digitalReadFast(pin);
}

extern void PinHandlers::readAnalog(const int pin, int &value) {
    value = analogRead(pin);
}

extern void PinHandlers::writeDigital(const int pin, int &value) {
    digitalWriteFast(pin, value);
}

extern void PinHandlers::writeAnalog(const int pin, int &value) {
    analogWrite(pin, value);
}