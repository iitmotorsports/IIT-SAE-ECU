#ifndef __MCUSTATES_H__
#define __MCUSTATES_H__

#include "MCU.h"
#include "State.h"

static struct Starting_t : State::State_t_proxy<Starting_t> {
private:
    bool firstSetup = true;
    int count = 5;

public:
    bool SetupOnce = false;
    bool enableSetup = true;
    State_t *nextState = this;
    State_t *errorState = this;
    elapsedMillis timeElapsed;

    State::ExitCode setup(void) {
        if (firstSetup) {
            pinMode(6, OUTPUT);
            digitalWrite(6, LOW); /* optional tranceiver enable pin */

            // FlexCan
            F_Can.begin();
            F_Can.setBaudRate(CONF_FLEXCAN_BAUD_RATE);
            setMailboxes();

            Pins::initialize();
            firstSetup = false;
        }
        count = 5;
        return State::NOERR;
    };

    State::ExitCode loop(void) {
        F_Can.events();
        if (timeElapsed >= 1000) {
            timeElapsed = timeElapsed - 1000;
            Serial.println(random(500));
            count--;
            // sendTestMessage(F_Can);
            // Serial.println(Pins::getPinValue(A7));
            // Pins::setPinValue(A6, random(1024));
        }
        Pins::update();
        if (count == 0)
            return State::DONE;

        return State::NOERR;
    };

    void error(void) {
        return;
    }
} Starting;
#endif // __MCUSTATES_H__