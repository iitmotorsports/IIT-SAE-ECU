#include "Front.h"
#include "MotorControl.h"
#include "Util.h"
#include "map"

namespace Front {

static struct State::State_t *states[] = {
    &ECUStates::Initialize_State,
    &ECUStates::PreCharge_State,
    &ECUStates::Idle_State,
    &ECUStates::Charging_State,
    &ECUStates::Button_State,
    &ECUStates::Driving_Mode_State,
    &ECUStates::FaultState,
};

std::map<uint32_t, struct State::State_t *> stateMap;
struct State::State_t *currentState;

void loadStateMap() {
    Log.i(ID, "Loading State Map");
    for (auto state : states) {
        Log.d(ID, "New State", TAG2NUM(state->getID()));
        Log.d(ID, "State Pointer", (uintptr_t)state);
        stateMap[TAG2NUM(state->getID())] = state;
    }
}

void updateCurrentState() {
    uint32_t currState = Pins::getCanPinValue(PINS_INTERNAL_STATE);
    currentState = stateMap[currState]; // returns NULL if not found
}

void updateStartLight() {
    static bool on = false;
    if (currentState == &ECUStates::Idle_State) {
        on = !on;
    } else {
        on = Pins::getCanPinValue(PINS_INTERNAL_START);
    }
    Log("start_light", "Start Light", on, true);
    Pins::setPinValue(PINS_FRONT_START_LIGHT, on);
}

} // namespace Front