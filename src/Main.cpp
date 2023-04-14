#include "ECU.h"
#include "ECUGlobalConfig.h"
#if CONF_ECU_POSITION == FRONT_ECU
#include "Front.h"
#endif

int main(void) {
    Serial.begin(CONF_ECU_BAUD_RATE);
    Log.initalizeSD();
    delay(CONF_ECU_INITAL_DELAY);
#if CONF_ECU_POSITION == BACK_ECU
    State::begin(ECUStates::Initialize_State);
#else
    Front::run();
#endif
    return 0;
}
