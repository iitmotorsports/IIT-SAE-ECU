#include "State.h"

static const char *TAG = "State Manager";

static struct UnhandledState_t : State::State_extend<UnhandledState_t> {
    const char *ID = "UNHANDLED STATE"; // TODO: test that this prints
} UnhandledState;

static State::ExitCode exitCode = State::NOERR;

extern State::ExitCode getExitCode() {
    return exitCode;
}

extern int State::begin(State_t &entry) {
    entry.trigger();

    while (1) {
        Log.i(TAG, "Begin State");
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
            Log.i(TAG, "State returned error code");
        currentState->error();
    }
    return 0;
}
