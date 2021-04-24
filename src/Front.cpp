#include "Front.h"
#include "ECUGlobalConfig.h"
#include "Echo.h"
#include "Heartbeat.h"
#include "Mirror.h"
#include "SerialCommand.h"
#include "unordered_map"

static LOG_TAG ID = "Front Teensy";

static elapsedMillis timeElapsed;
static elapsedMillis timeElapsedMidHigh;
static elapsedMillis timeElapsedMidLow;
static elapsedMillis timeElapsedLong;
static Canbus::Buffer MC0_RPM_Buffer(ADD_MC0_RPM);
static Canbus::Buffer MC1_RPM_Buffer(ADD_MC1_RPM);
static Canbus::Buffer MC0_VOLT_Buffer(ADD_MC0_VOLT);
static Canbus::Buffer MC1_VOLT_Buffer(ADD_MC1_VOLT);
static Canbus::Buffer MC0_CURR_Buffer(ADD_MC0_CURR);
static Canbus::Buffer MC1_CURR_Buffer(ADD_MC1_CURR);
static Canbus::Buffer BMS_DATA_Buffer(ADD_BMS_DATA);
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
    return BMS_DATA_Buffer.getByte(4); // Byte 4: BMS State of charge buffer
}

static uint32_t BMSVOLT() {
    return BMS_DATA_Buffer.getShort(2); // Byte 2-3: BMS Immediate voltage
}

static int32_t BMSAMP() {
    return BMS_DATA_Buffer.getShort(0); // Byte 0-1: BMS Immediate amperage
}

static uint16_t MC0Voltage() {
    return MC0_VOLT_Buffer.getShort(0) / 10; // Bytes 0-1: DC BUS MC Voltage
}

static uint16_t MC1Voltage() {
    return MC1_VOLT_Buffer.getShort(0) / 10; // Bytes 0-1: DC BUS MC Voltage
}

static uint32_t MCPowerValue() {                    // IMPROVE: get power value using three phase values, or find a power value address
    int16_t MC1_CURR = MC0_CURR_Buffer.getShort(6); // Bytes 6-7: DC BUS MC Current
    int16_t MC0_CURR = MC1_CURR_Buffer.getShort(6); // Bytes 6-7: DC BUS MC Current
    int MC0_PWR = MC0Voltage() * MC0_CURR;
    int MC1_PWR = MC1Voltage() * MC1_CURR;
    return (MC0_PWR + MC1_PWR) / 1000; // Sending kilowatts
}

// receive rpm of MCs, interpret, then send to from teensy for logging
static int32_t motorSpeed() {
    int16_t MC_Rpm_Val_0 = MC0_RPM_Buffer.getShort(2); // Bytes 2-3 : Angular Velocity
    int16_t MC_Rpm_Val_1 = MC1_RPM_Buffer.getShort(2); // Bytes 2-3 : Angular Velocity
    float MC_Spd_Val_0 = wheelRadius * 2 * 3.1415926536 / 60 * MC_Rpm_Val_0;
    float MC_Spd_Val_1 = wheelRadius * 2 * 3.1415926536 / 60 * MC_Rpm_Val_1;
    return (MC_Spd_Val_0 + MC_Spd_Val_1) / 2;
}

static void loadBuffers() {
    Log.i(ID, "Loading Buffers");
    MC0_RPM_Buffer.init();
    MC1_RPM_Buffer.init();
    MC0_VOLT_Buffer.init();
    MC1_VOLT_Buffer.init();
    MC0_CURR_Buffer.init();
    MC1_CURR_Buffer.init();
    BMS_DATA_Buffer.init();
}

static void updateCurrentState() {
    uint32_t currState = Pins::getCanPinValue(PINS_INTERNAL_STATE);
    currentState = stateMap[currState]; // returns NULL if not found
    Log(ID, "Current State", currState);
}

static void loadStateMap() {
    Log.i(ID, "Loading State Map");
    for (auto state : states) {
        Log.d(ID, "New State", TAG2NUM(state->getID()));
        Log.d(ID, "State Pointer", (uintptr_t)state);
        stateMap[TAG2NUM(state->getID())] = state;
    }
}

static void setChargeSignal() {
    Pins::setInternalValue(PINS_INTERNAL_CHARGE_SIGNAL, currentState == &ECUStates::Idle_State);
}

static void pushCanMessage() {
    uint32_t address;
    char buffer[8];
    size_t c = 0;
    c += Serial.readBytes((char *)&address, 4);
    c += Serial.readBytes(buffer, 8);
    if (c != 12) {
        Log.w(ID, "Did not read correct number of Bytes, not pushing message", 12);
        return;
    }
    Log.d(ID, "Pushing Message", address);
    Canbus::sendData(address, (uint8_t *)buffer);
}

static void toggleCanbusSniffer() {
    static bool enabled = false;
    Canbus::enableCanbusSniffer((enabled = !enabled));
}

static void sendEchoMessage() {
    uint32_t delay;
    uint32_t address;
    char buffer[8];
    size_t c = 0;
    c += Serial.readBytes((char *)&delay, 4);
    c += Serial.readBytes((char *)&address, 4);
    c += Serial.readBytes(buffer, 8);
    if (c != 20) {
        Log.w(ID, "Did not read correct number of Bytes, not pushing message", 20);
        return;
    }
    Log.d(ID, "Pushing Message", address);
    Echo::echo(delay, address, (uint8_t *)buffer);
}

void Front::run() {
    Log.i(ID, "Teensy 3.6 SAE FRONT ECU Initalizing");
    Log.i(ID, "Setting up Canbus");
    Canbus::setup(); // allocate and organize addresses
    Log.i(ID, "Initalizing Pins");
    Pins::initialize(); // setup predefined pins
    Log.i(ID, "Enabling Logging relay");
    Logging::enableCanbusRelay(); // Allow logging though canbus
    loadBuffers();
    loadStateMap();

    Log.i(ID, "Setting commands");
    Command::setCommand(COMMAND_ENABLE_CHARGING, setChargeSignal);
    Command::setCommand(COMMAND_SEND_CANBUS_MESSAGE, pushCanMessage);
    Command::setCommand(COMMAND_TOGGLE_CANBUS_SNIFF, toggleCanbusSniffer);
    Command::setCommand(COMMAND_SEND_ECHO, sendEchoMessage);

    Heartbeat::beginReceiving();
#ifdef CONF_ECU_DEBUG
    Mirror::setup();
#endif

    static int testValue = 0;
    static bool hasBeat = false;

    while (true) {
        if (timeElapsed >= 20) { // High priority updates
            timeElapsed = 0;

            Command::receiveCommand();

            testValue = Pins::getPinValue(PINS_FRONT_PEDAL1); // TODO: remove test pedal value

            Log(ID, "Current Motor Speed:", motorSpeed() + testValue);
        }
        if (timeElapsedMidHigh >= 200) { // MedHigh priority updates
            timeElapsedMidHigh = 0;
            updateCurrentState();
            hasBeat = Heartbeat::checkBeat();
        }
        if (timeElapsedMidLow >= 500) { // MedLow priority updates
            timeElapsedMidLow = 0;
            static bool on = false;
            if (hasBeat && (currentState == &ECUStates::Idle_State)) {
                on = !on;
            } else {
                on = 0;
            }
            Log(ID, "Start Light", on);
            Pins::setPinValue(PINS_FRONT_START_LIGHT, on);
        }
        if (timeElapsedLong >= 800) { // Low priority updates
            timeElapsedLong = 0;
            Pins::setPinValue(PINS_FRONT_BMS_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_BMS_FAULT));
            Pins::setPinValue(PINS_FRONT_IMD_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_IMD_FAULT));

            Log(ID, "MC0 Voltage:", MC0Voltage());
            Log(ID, "MC1 Voltage:", MC1Voltage());
            Log(ID, "Current Power Value:", MCPowerValue()); // Canbus message from MCs
            Log(ID, "BMS State Of Charge Value:", BMSSOC()); // Canbus message
            Log(ID, "BMS Immediate Voltage:", BMSVOLT());    // Canbus message
            Log(ID, "BMS Pack Average Current:", BMSAMP());    // Canbus message
            Log(ID, "Fault State", Pins::getCanPinValue(PINS_INTERNAL_GEN_FAULT));
            // TODO: send MC temps, Motor temps, aero angles, DC BUS current
        }
    }
}