#include "ECU.h"
#include "ECUGlobalConfig.h"
#if CONF_ECU_POSITION == FRONT_ECU
#include "Front.h"
#endif

// NOTE: Consider using back teensy as a dumb relay and have front teensy only be used for actual logic

int main(void) {
    Serial.begin(CONF_ECU_BAUD_RATE);
    delay(CONF_ECU_INITAL_DELAY);
#if CONF_ECU_POSITION == BACK_ECU
    State::begin(ECUStates::Initialize_State);
#else
    Front::run();
#endif
    return 0;
}
