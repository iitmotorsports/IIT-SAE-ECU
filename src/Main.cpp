// #include "ECU.h"

// #include "threadTest.h"

// int main(void) {
//     Serial.begin(CONF_ECU_BAUD_RATE);
//     threadTest();
// }

#include "ECU.h"
#include "ECUGlobalConfig.h"
#if CONF_ECU_POSITION == FRONT_ECU
#include "Front.h"
#endif

#include "testmod.h"
#include "threadTest.h"

int main(void) {
    Serial.begin(CONF_ECU_BAUD_RATE);
    Thread::addThread([]() { testModules(); }, 4096);
    threadTest();
#if CONF_ECU_POSITION == BACK_ECU
    State::begin(ECUStates::Initialize_State);
#else
    Front::run();
#endif
    return 0;
}
