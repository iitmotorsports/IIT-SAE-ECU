#include "State.h"

static LOG_TAG TAG = "State Manager";

static State::State_t *currentState;

void State::setNextState(State_t *state) {
    currentState = state;
}

static struct UnhandledState_t : State::State_extend<UnhandledState_t> {
    bool SetupOnce = true;
    LOG_TAG ID = "UNHANDLED STATE"; // TODO: test that this prints

    State::ExitCode setup(void) {
        Log.f(ID, "UNHANDLED STATE!");
        return State::STOP;
    };

} UnhandledState;

State::State_t *State::State_t::nextState = &UnhandledState;
State::State_t *State::State_t::errorState = &UnhandledState;

static State::ExitCode exitCode = State::NOERR;

State::ExitCode getExitCode() {
    return exitCode;
}

int State::begin(State_t &entry) {
    setNextState(&entry);

    while (exitCode != STOP) {
        Log.d(TAG, "Start");
        exitCode = currentState->runSetup();

        if (exitCode != NOERR)
            goto ERRORED;

        do { // TODO: what happens if runtime error?
            exitCode = currentState->loop();
        } while (exitCode == NOERR);

        if (exitCode == DONE) {
            Log.d(TAG, "Next");
            currentState->next();
        } else if (exitCode != NOERR) {
        ERRORED:
            Log.e(TAG, "State returned error code", exitCode);
            currentState->error();
        }
    }

    Log.f(TAG, "UNABLE TO CONTINUE");
    return 0;
}
