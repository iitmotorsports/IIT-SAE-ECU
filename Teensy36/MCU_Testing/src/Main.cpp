#include "MCU.h"
#include "Config.def"

int main(void) {
    Serial.begin(CONF_TEENSY_BAUD_RATE);
    delay(CONF_TEENSY_INITAL_DELAY);
    State::begin(Starting);
    return 0;
}
