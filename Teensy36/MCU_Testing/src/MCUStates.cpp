#include "MCUStates.h"

State::State_t *MCUStates::Initialize_t::linkedStates[] = {&Bounce};
State::ExitCode MCUStates::Initialize_t::setup(void) {
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

State::ExitCode MCUStates::Initialize_t::loop(void) {

    if (timeElapsed >= 500) {
        timeElapsed = timeElapsed - 500;
        count--;
        // sendTestMessage(F_Can);
        Log(ID, "A7 Pin Value:", Pins::getPinValue(0));
        Pins::setPinValue(A6, random(1024));
    }
    Pins::update();
    if (count == 0)
        return State::DONE;

    return State::NOERR;
};

State::State_t *MCUStates::Bounce_t::linkedStates[] = {&Bounce};
State::ExitCode MCUStates::Bounce_t::loop(void) {
    delay(500);
    Log.i(ID, "Bounce!");
    linkedStates[0] = State::getLastState();
    delay(500);
    return State::DONE;
}