#include <stdint.h>
#include <stdlib.h>

#include "FlexCAN_T4.h"
#include "WProgram.h"

#include "State.h"
#include "Pins.h"

static struct Starting_t : State::State_extend<Starting_t> {
private:
    elapsedMillis timeElapsed;
    bool firstSetup = true;
    int count = 5;

public:
    bool SetupOnce = false;
    bool enableSetup = true;
    const char *ID = "Starting";
    // State_t *nextState = this;
    // State_t *errorState = this;

    State::ExitCode setup(void) {
        if (firstSetup) {
            pinMode(6, OUTPUT);
            digitalWrite(6, LOW); /* optional tranceiver enable pin */

            Pins::initialize();
            firstSetup = false;
            Log.i(ID, "Finished inital Setup");
        }
        count = 5;
        return State::NOERR;
    };

    State::ExitCode loop(void) {

        if (timeElapsed >= 1000) {
            timeElapsed = timeElapsed - 1000;
            Log(ID, "%u", random(300));
            count--;
            // sendTestMessage(F_Can);
            Serial.println(Pins::getPinValue(A7));
            Pins::setPinValue(A6, random(1024));
        }
        Pins::update();
        if (count == 0)
            return State::DONE;

        return State::NOERR;
    };
} Starting;