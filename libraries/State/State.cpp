/**
 * @file State.cpp
 * @author IR
 * @brief State source file
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */

// @cond
#include "State.h"
#include "Log.h"
#include "Pins.h"
#include "WProgram.h"

static LOG_TAG TAG = "State Manager";

static State::State_t *lastState;
static State::State_t *currentState;
static int currentNotifyCode = 0;

static void setNextState(State::State_t *state) {
    lastState = currentState;
    currentState = state;
    currentState->notifyCode = currentNotifyCode;
}

static struct UnhandledState_t : State::State_t {
    LOG_TAG ID = "UNHANDLED STATE";

    State_t *run(void) {
        Log.f(ID, "UNHANDLED STATE!");
        notify(State::E_FATAL);
        return this;
    };

} UnhandledState;

State::State_t *State::State_t::run() {
    return &UnhandledState;
}

void State::State_t::notify(int code) {
    currentNotifyCode = code;
}

int State::State_t::getNotify() {
    return this->notifyCode;
}

State::State_t *State::State_t::getLastState() {
    return lastState;
}

LOG_TAG State::State_t::getID() {
    return (LOG_TAG)420;
}

void State::begin(State_t &entry) {
    State_t *startingState = &entry;
    setNextState(&entry);

    Log.d(TAG, "Starting State Machine");
    while (currentNotifyCode != State::E_FATAL) {
        currentNotifyCode = 0;
        Log.d(TAG, "Start", TAG2NUM(currentState->getID())); // Note: cannot send initalizing state as pins are not defined yet to transmit message
        Pins::setInternalValue(PINS_INTERNAL_STATE, TAG2NUM(currentState->getID()));

        State_t *queuedState = currentState->run();

        Log.d(TAG, "State returned code", currentNotifyCode);

        if (currentNotifyCode == State::E_RESTART) {
            setNextState(startingState);
        } else {
            setNextState(queuedState);
        }

        delay(1000);
    }

    Log.f(TAG, "STATE MACHINE STOPPED");
}
// @endcond