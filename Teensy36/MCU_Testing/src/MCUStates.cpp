#include "MCUStates.h"

State::ExitCode Starting_t::setup(void) {
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

State::ExitCode Starting_t::loop(void) {

    if (timeElapsed >= 5000) {
        timeElapsed = timeElapsed - 5000;
        count--;
        // sendTestMessage(F_Can);
        Log(ID, "Oh why hello there");
        delay(10);
        Log(ID, "A7 Pin Value: ", Pins::getPinValue(A7));
        Pins::setPinValue(A6, random(1024));
    }
    Pins::update();
    if (count == 0)
        return State::DONE;

    return State::NOERR;
};