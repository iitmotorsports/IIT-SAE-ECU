#include <stdint.h>
#include <stdlib.h>

#include "FlexCAN_T4.h"
#include "WProgram.h"

#include "Pins.h"
#include "State.h"

namespace MCUStates {

static struct Initialize_t : State::State_extend<Initialize_t> {
private:
    elapsedMillis timeElapsed;
    bool firstSetup = true;
    int count = 5;

public:
    bool SetupOnce = false;
    bool enableSetup = true;
    LOG_TAG ID = "Teensy Start";
    State_t *nextState = this;
    State_t *errorState = this;

    State::ExitCode setup(void);
    State::ExitCode loop(void);

} Initialize;

} // namespace MCUState