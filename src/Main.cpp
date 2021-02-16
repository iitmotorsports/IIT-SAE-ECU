#include "ECU.h"
#include "ECUGlobalConfig.h"
#include "Front.h"

// NOTE: Consider using back teensy as a dumb relay and have front teensy only be used for actual logic

int main(void) {
    Serial.begin(CONF_TEENSY_BAUD_RATE);
    delay(CONF_TEENSY_INITAL_DELAY);
#if CONF_ECU_POSITION == BACK_ECU
    State::begin(ECUStates::Initialize);
#else
    Front::run();
#endif
    return 0;
}
