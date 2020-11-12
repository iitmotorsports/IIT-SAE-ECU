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
#include "State.h"
#include "WProgram.h"

static LOG_TAG TAG = "State Manager";

static State::State_t *lastState;
static State::State_t *currentState;
static int notifyCode = 0;

static void setNextState(State::State_t *state) {
    lastState = currentState;
    currentState = state;
}

/**
 * @brief Internal state used for invalid states
 */
static struct UnhandledState_t : State::State_t {
    LOG_TAG ID = "UNHANDLED STATE";

    State_t *run(void) {
        Log.f(ID, "UNHANDLED STATE!");
        State::notify(State::E_FATAL);
        return this;
    };

} UnhandledState;

State::State_t *State::State_t::run() {
    return &UnhandledState;
}

void State::notify(int notify) {
    notifyCode = notify;
}

int State::State_t::getNotify() {
    return this->notify;
}

State::State_t *State::getLastState() {
    return lastState;
}

void State::begin(State_t &entry) {
    setNextState(&entry);

    while (notifyCode != State::E_FATAL) {
        notifyCode = 0;
        Log.d(TAG, "Start");
        setNextState(currentState->run());
        currentState->notify = notifyCode;
        Log.d(TAG, "State returned code", notifyCode);
    }

    Log.f(TAG, "STATE MACHINE STOPPED");
}
