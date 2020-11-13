#include "Handlers.hpp"
#include "WProgram.h"

void PinHandlers::null(const int pin, int &value) {
}

void PinHandlers::readDigital(const int pin, int &value) {
    value = digitalReadFast(pin);
}

void PinHandlers::readAnalog(const int pin, int &value) {
    value = analogRead(pin);
}

void PinHandlers::writeDigital(const int pin, int &value) {
    digitalWriteFast(pin, value);
}

void PinHandlers::writeAnalog(const int pin, int &value) {
    analogWrite(pin, value);
}