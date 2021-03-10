#include "Front.h"
#include "ECUGlobalConfig.h"
#include "unordered_map"

static LOG_TAG ID = "Front Teensy";

static elapsedMillis timeElapsed;
static elapsedMillis timeElapsedMid;
static elapsedMillis timeElapsedLong;
static uint8_t *MC0_RPM_Buffer;
static uint8_t *MC1_RPM_Buffer;
static uint8_t *MC0_VOLT_Buffer;
static uint8_t *MC1_VOLT_Buffer;
static uint8_t *MC0_CURR_Buffer;
static uint8_t *MC1_CURR_Buffer;
static uint8_t *BMS_SOC_Buffer;
static constexpr float wheelRadius = 1.8; // TODO: Get car wheel radius

static struct State::State_t *states[] = {
    &ECUStates::Initialize_State,
    &ECUStates::PreCharge_State,
    &ECUStates::Idle_State,
    &ECUStates::Charging_State,
    &ECUStates::Button_State,
    &ECUStates::Driving_Mode_State,
    &ECUStates::FaultState,
};
std::unordered_map<uint32_t, struct State::State_t *> stateMap;
static struct State::State_t *currentState;

static uint32_t BMSSOC() {
    Canbus::setSemaphore(ADD_BMS_SOC);
    return *(uint8_t *)(BMS_SOC_Buffer + 4); // Byte 4: BMS State of charge buffer
    Canbus::clearSemaphore();
}

static uint32_t powerValue() { // IMPROVE: get power value using three phase values, or find a power value address
    Canbus::setSemaphore(ADD_MC0_VOLT);
    int16_t MC0_VOLT = *((int16_t *)(MC0_VOLT_Buffer)) / 10; // Bytes 0-1: DC BUS MC Voltage
    Canbus::setSemaphore(ADD_MC0_CURR);
    int16_t MC0_CURR = *((int16_t *)(MC0_CURR_Buffer + 6)) / 10; // Bytes 6-7: DC BUS MC Current
    int MC0_PWR = MC0_VOLT * MC0_CURR;
    Canbus::setSemaphore(ADD_MC1_VOLT);
    int16_t MC1_VOLT = *((int16_t *)(MC1_VOLT_Buffer)) / 10; // Bytes 0-1: DC BUS MC Voltage
    Canbus::setSemaphore(ADD_MC1_CURR);
    int16_t MC1_CURR = *((int16_t *)(MC1_CURR_Buffer + 6)) / 10; // Bytes 6-7:
    int MC1_PWR = MC1_VOLT * MC1_CURR;
    Canbus::clearSemaphore();
    return (MC0_PWR + MC1_PWR) / 1000; // Sending kilowatts
}

static int32_t motorSpeed() {
    // receive rpm of MCs, interpret, then send to from teensy for logging
    Canbus::setSemaphore(ADD_MC0_RPM);
    int16_t MC_Rpm_Val_0 = *(int16_t *)(MC0_RPM_Buffer + 2); // Bytes 2-3 : Angular Velocity
    Canbus::setSemaphore(ADD_MC1_RPM);
    int16_t MC_Rpm_Val_1 = *(int16_t *)(MC1_RPM_Buffer + 2); // Bytes 2-3 : Angular Velocity
    Canbus::clearSemaphore();
    float MC_Spd_Val_0 = wheelRadius * 2 * 3.1415926536 / 60 * MC_Rpm_Val_0;
    float MC_Spd_Val_1 = wheelRadius * 2 * 3.1415926536 / 60 * MC_Rpm_Val_1;
    return (MC_Spd_Val_0 + MC_Spd_Val_1) / 2;
}

static void readSerial() {
    static uint8_t serialData = 0;
    if (Serial.available()) {
        serialData = Serial.read();
        Log.d(ID, "Data received: ", serialData);
    }
    if (serialData == COMMAND_ENABLE_CHARGING)
        Pins::setInternalValue(PINS_INTERNAL_CHARGE_SIGNAL, currentState == &ECUStates::Idle_State);
}

static void getBuffers() {
    Log.i(ID, "Loading Buffers");
    MC0_RPM_Buffer = Canbus::getBuffer(ADD_MC0_RPM);
    MC1_RPM_Buffer = Canbus::getBuffer(ADD_MC1_RPM);
    MC0_VOLT_Buffer = Canbus::getBuffer(ADD_MC0_VOLT);
    MC1_VOLT_Buffer = Canbus::getBuffer(ADD_MC1_VOLT);
    MC0_CURR_Buffer = Canbus::getBuffer(ADD_MC0_CURR);
    MC1_CURR_Buffer = Canbus::getBuffer(ADD_MC1_CURR);
    BMS_SOC_Buffer = Canbus::getBuffer(ADD_BMS_SOC);
}

static void updateCurrentState() {
    uint32_t currState = Pins::getCanPinValue(PINS_INTERNAL_STATE);
    currentState = stateMap[currState]; // returns NULL if not found
    Log.i(ID, "Current State", currState);
}

static void loadStateMap() {
    Log.i(ID, "Loading State Map");
    for (auto state : states) {
        Log.d(ID, "New State", TAG2NUM(state->getID()));
        Log.d(ID, "State Pointer", (uintptr_t)state);
        stateMap[TAG2NUM(state->getID())] = state;
    }
}

void Front::run() {
    Log.i(ID, "Teensy 3.6 SAE FRONT ECU Initalizing");
    Log.i(ID, "Setting up Canbus");
    Canbus::setup(); // allocate and organize addresses
    Log.i(ID, "Initalizing Pins");
    Pins::initialize(); // setup predefined pins
    Log.i(ID, "Enabling Logging relay");
    Logging::enableCanbusRelay(); // Allow logging though canbus
    getBuffers();
    loadStateMap();

    while (true) {
        if (timeElapsed >= 20) { // High priority updates
            timeElapsed = 0;

            readSerial();

            Log.i(ID, "Current Motor Speed:", motorSpeed() + Pins::getPinValue(PINS_FRONT_PEDAL1)); // TODO: remove test pedal value
        }
        if (timeElapsedMid >= 200) { // Med priority updates
            timeElapsedMid = 0;
            updateCurrentState();
        }
        if (timeElapsedLong >= 800) { // Low priority updates
            timeElapsedLong = 0;

            Pins::setPinValue(PINS_FRONT_START_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_START));
            Pins::setPinValue(PINS_FRONT_BMS_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_BMS_FAULT));
            Pins::setPinValue(PINS_FRONT_IMD_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_IMD_FAULT));

            // TODO: Send both mc voltages

            Log.i(ID, "Current Power Value:", powerValue() + Pins::getPinValue(PINS_FRONT_PEDAL1));   // Canbus message from MCs
            Log.i(ID, "BMS State Of Charge Value:", BMSSOC() + Pins::getPinValue(PINS_FRONT_PEDAL1)); // Canbus message
            Log.i(ID, "Fault State", Pins::getCanPinValue(PINS_INTERNAL_GEN_FAULT));
            if (currentState != &ECUStates::Charging_State || currentState != &ECUStates::Idle_State) {
                Pins::setInternalValue(PINS_INTERNAL_CHARGE_SIGNAL, LOW);
            }
        }
    }
}