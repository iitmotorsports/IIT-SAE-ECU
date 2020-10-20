/**
 * State controls what can be done and when
 * 
 * CAN BUS faults
 *  Are we in a state that ignores these faults?
 *  Maybe we need to check for specific faults regardless of what fault(hard/soft)?
 *      We must pass data(the fault bytes) to State incase we do
 *      In the case of a unrecoverable fault, disable interrupts and do what is needed
 *      Each state can have their own way of managing faults
 * 
 * Notifying tablet
 *  Are we on a teensy that can notify the tablet?
 *      If so, just push the address with the appropriate status code if somthing of interest happens
 *      If the tablet receives a status code of a diffrent state then that means we have changed states
 * 
 * Sequences
 *  Specific sequences that must be carried out in a state can be implemented here
 *  Interrupts can be disabled and any faults can be checked for manually
 * 
 */

#ifndef __STATE_H__
#define __STATE_H__

#include <stdint.h>
#include <stdlib.h>
#include <typeinfo>

#include "config.def"

namespace State {

enum ExitCode {
    NOERR,
    DONE,
    ERROR,
    FAULT
};

struct State_t;

static State_t *currentState;
static ExitCode exitCode = NOERR;

// Return last exitCode; for error handling states
extern ExitCode getExitCode() {
    return exitCode;
}

struct State_t {

    bool SetupOnce = false;
    bool enableSetup = true;
    State_t *nextState;  // Next State to goto if loop exits w/ code DONE
    State_t *errorState; // State to goto if state exits not with NOERR

    void trigger() {
        currentState = this;
    }

    virtual ExitCode runSetup();
    virtual void next(void);
    virtual void error(void);

    virtual ExitCode setup(void);
    virtual ExitCode loop(void);
};

template <typename Derived>
struct State_t_proxy : State_t {

    virtual ExitCode runSetup() {
        Derived *p = static_cast<Derived *>(this);
        if (p->enableSetup) {
            Serial.println("[Info] Setup State");
            ExitCode exitCode = setup();
            if (exitCode != NOERR)
                return exitCode;
            if (p->SetupOnce) {
                Serial.println("[Info] Setup State Once");
                p->enableSetup = false;
            }
        }
        return NOERR;
    }

    virtual void next(void) {
        Derived *p = static_cast<Derived *>(this);
        p->nextState->trigger();
    };

    virtual void error(void) {
        Derived *p = static_cast<Derived *>(this);
        p->errorState->trigger();
    };
};

static int begin(State_t &entry) {
    entry.trigger();
    Serial.begin(CONF_TEENSY_BAUD_RATE);
    delay(CONF_TEENSY_INITAL_DELAY);

    while (1) {
        Serial.println("[Info] Begin State");
        exitCode = NOERR;

        if (currentState->runSetup() != NOERR)
            goto ERRORED;

        do { // TODO: what happens if runtime error?
            exitCode = currentState->loop();
        } while (exitCode == NOERR);

        if (exitCode == DONE)
            currentState->next();
        else if (exitCode != NOERR)
        ERRORED:
            Serial.println("[Error] State error");
        currentState->error();
    }
    return 0;
}

} // namespace State

#endif // __STATE_H__