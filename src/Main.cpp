#include "ECU.h"
#include "ECUGlobalConfig.h"

int main(void) {
    Serial.begin(CONF_TEENSY_BAUD_RATE);
    delay(CONF_TEENSY_INITAL_DELAY);
    State::begin(ECUStates::Initialize);
    return 0;
}
