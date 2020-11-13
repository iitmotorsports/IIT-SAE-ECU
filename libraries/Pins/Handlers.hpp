#ifndef __ECU_PINHANDLERS_H__
#define __ECU_PINHANDLERS_H__

#include "PinConfig.def"

#define ANALOGOUTPUT PinHandlers::writeAnalog
#define ANALOGINPUT PinHandlers::readAnalog
#define DIGITALOUTPUT PinHandlers::writeDigital
#define DIGITALINPUT PinHandlers::readDigital

namespace PinHandlers {

void null(const int pin, int &value);
void readDigital(const int pin, int &value);
void readAnalog(const int pin, int &value);
void writeDigital(const int pin, int &value);
void writeAnalog(const int pin, int &value);

} // namespace PinHandlers

#endif // __ECU_PINHANDLERS_H__