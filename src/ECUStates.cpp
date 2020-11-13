#include "ECUStates.hpp"

State::State_t *ECUStates::Initialize_t::run(void) {
    if (firstSetup) {
        pinMode(6, OUTPUT);
        digitalWrite(6, LOW); /* optional transceiver enable pin */

        Pins::initialize();
        firstSetup = false;
        Log.i(ID, "Finished inital Setup");
    }

    elapsedMillis timeElapsed;
    int count = 5;

    while (true) {
        if (timeElapsed >= 500) {
            timeElapsed = timeElapsed - 500;
            count--;
            // sendTestMessage(F_Can);
            Log(ID, "A7 Pin Value:", Pins::getPinValue(0));
            Pins::setPinValue(A6, random(1024));
        }
        Pins::update();
        if (count == 0)
            return &ECUStates::Logger;
    }
};

State::State_t *ECUStates::Logger_t::run(void) {

    static elapsedMillis timeElapsed;

    if (timeElapsed >= 2000) {
        timeElapsed = timeElapsed - 2000;
        Log(ID, "A7 Pin Value:", Pins::getPinValue(0));
        Log("FAKE ID", "A7 Pin Value:");
        Log(ID, "whaAAAT?");
        Log(ID, "", 0xDEADBEEF);
        Log(ID, "Notify code: ", getNotify());
    }

    return &ECUStates::Bounce;
};

State::State_t *ECUStates::Bounce_t::run(void) {
    delay(250);
    Log.i(ID, "Bounce!");
    State::notify(random(100));
    delay(250);
    return State::getLastState();
}