#include "ECU.h"
#include "ECUGlobalConfig.h"

// NOTE: Consider using back teensy as a dumb relay and have front teensy only be used for actual logic

int main(void) {
    Serial.begin(CONF_TEENSY_BAUD_RATE);
    delay(CONF_TEENSY_INITAL_DELAY);
    State::begin(ECUStates::Initialize);
    return 0;
}
