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
static Canbus::Buffer MC0_TEMP2_Buffer(ADD_MC0_TEMP2);
static Canbus::Buffer MC1_TEMP2_Buffer(ADD_MC1_TEMP2);
static Canbus::Buffer MC0_TEMP3_Buffer(ADD_MC0_TEMP3);
static Canbus::Buffer MC1_TEMP3_Buffer(ADD_MC1_TEMP3);

static Canbus::Buffer BMS_DATA_Buffer(ADD_BMS_DATA);
static Canbus::Buffer BMS_BATT_TEMP_Buffer(ADD_BMS_BATT_TEMP);
static Canbus::Buffer BMS_CURR_LIMIT_Buffer(ADD_BMS_CURR_LIMIT);

static constexpr float wheelRadius = CONF_CAR_WHEEL_RADIUS;

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

static uint8_t BMSSOC() {                   // Percent
    return BMS_DATA_Buffer.getUByte(4) / 2; // Byte 4: BMS State of charge buffer
}

static uint16_t BMSVOLT() {
    return BMS_DATA_Buffer.getShort(2) / 10; // Byte 2-3: BMS Immediate voltage
}

static uint16_t BMSAMP() {
    return BMS_DATA_Buffer.getShort(0); // Byte 0-1: BMS Immediate amperage
}

static uint8_t BMSTempHigh() {
    return BMS_BATT_TEMP_Buffer.getByte(4); // Byte 4: BMS Highest Battery Temp
}

static uint8_t BMSTempLow() {
    return BMS_BATT_TEMP_Buffer.getByte(5); // Byte 5: BMS Lowest Battery Temp
}

static uint16_t BMSDischargeCurrentLimit() {
    return BMS_CURR_LIMIT_Buffer.getShort(0); // Byte 0-1: BMS Discharge Current Limit
}

static uint16_t BMSChargeCurrentLimit() {
    return BMS_CURR_LIMIT_Buffer.getShort(2); // Byte 2-3: BMS Charge Current Limit
}

static uint16_t MC0BoardTemp() {
    return MC0_TEMP2_Buffer.getShort(0) / 10; // Bytes 0-1: Temperature of Control Board
}

static uint16_t MC1BoardTemp() {
    return MC1_TEMP2_Buffer.getShort(0) / 10; // Bytes 0-1: Temperature of Control Board
}

static uint16_t MC0MotorTemp() {
    return MC0_TEMP3_Buffer.getShort(4) / 10; // Bytes 4-5: Filtered temperature value from the motor temperature sensor
}

static uint16_t MC1MotorTemp() {
    return MC1_TEMP3_Buffer.getShort(4) / 10; // Bytes 4-5: Filtered temperature value from the motor temperature sensor
}

static uint16_t MC0Voltage() {
    return MC0_VOLT_Buffer.getShort(0) / 10; // Bytes 0-1: DC BUS MC Voltage
}

static uint16_t MC1Voltage() {
    return MC1_VOLT_Buffer.getShort(0) / 10; // Bytes 0-1: DC BUS MC Voltage
}

static uint16_t MC0Current() {
    return MC0_CURR_Buffer.getShort(6) / 10; // Bytes 6-7: DC BUS MC Current
}

static uint16_t MC1Current() {
    return MC0_CURR_Buffer.getShort(6) / 10; // Bytes 6-7: DC BUS MC Current
}

static uint32_t MCPowerValue() { // IMPROVE: get power value using three phase values, or find a power value address
    int MC0_PWR = MC0Voltage() * MC0Current();
    int MC1_PWR = MC1Voltage() * MC1Current();
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
    MC0_TEMP2_Buffer.init();
    MC1_TEMP2_Buffer.init();
    MC0_TEMP3_Buffer.init();
    MC1_TEMP3_Buffer.init();

    BMS_DATA_Buffer.init();
    BMS_BATT_TEMP_Buffer.init();
    BMS_CURR_LIMIT_Buffer.init();
}

static void updateCurrentState() {
    uint32_t currState = Pins::getCanPinValue(PINS_INTERNAL_STATE);
    currentState = stateMap[currState]; // returns NULL if not found
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

static void toggleMotorDirection() {
    static bool reverse = false;
    reverse = !reverse;
    Pins::setInternalValue(PINS_INTERNAL_REVERSE, reverse);
}

#ifdef TESTING
static void testValues() {
    if (timeElapsed >= 20) { // High priority updates
        timeElapsed = 0;
        static uint32_t speed = 300;
        static bool direction = true;
        if (speed < 50)
            direction = false;
        else if (speed > 250)
            direction = true;
        speed += (direction ? -1 : 1) * random(10);
        Log(ID, "Current Motor Speed:", speed);
    }
    if (timeElapsedMidHigh >= 200) { // MedHigh priority updates
        timeElapsedMidHigh = 0;
        updateCurrentState();
        Log(ID, "Current State", currState);
    }
    if (timeElapsedMidLow >= 500) { // MedLow priority updates
        timeElapsedMidLow = 0;
        static bool on = random(100) > 50;
        Log(ID, "Start Light", on);
    }
    if (timeElapsedLong >= 800) { // Low priority updates
        timeElapsedLong = 0;

        // Motor controllers
        Log(ID, "MC0 DC BUS Voltage:", random(200));
        Log(ID, "MC1 DC BUS Voltage:", random(200));
        Log(ID, "MC0 DC BUS Current:", random(200));
        Log(ID, "MC1 DC BUS Current:", random(200));
        Log(ID, "MC0 Board Temp:", random(200));
        Log(ID, "MC1 Board Temp:", random(200));
        Log(ID, "MC0 Motor Temp:", random(200));
        Log(ID, "MC1 Motor Temp:", random(200));
        Log(ID, "MC Current Power:", random(200));

        // BMS
        Log(ID, "BMS State Of Charge:", random(100));
        Log(ID, "BMS Immediate Voltage:", random(200));
        Log(ID, "BMS Pack Average Current:", random(200));
        Log(ID, "BMS Pack Highest Temp:", random(200));
        Log(ID, "BMS Pack Lowest Temp:", random(200));
        Log(ID, "BMS Discharge current limit:", random(200));
        Log(ID, "BMS Charge current limit:", random(200));

        // General
        Log(ID, "Fault State", random(100) > 50);
    }
}
#endif

void blinkStart() {
    Pins::setPinValue(PINS_FRONT_START_LIGHT, 1);
    delay(500);
    Pins::setPinValue(PINS_FRONT_START_LIGHT, 0);
    delay(500);
}

void Front::run() {
    Log.i(ID, "Teensy 3.6 SAE FRONT ECU Initalizing");

    Log.i(ID, "Setting up Canbus");
    Canbus::setup(); // allocate and organize addresses
    Log.i(ID, "Initalizing Pins");
    Pins::initialize(); // setup predefined pins
#ifndef CONF_LOGGING_ASCII_DEBUG
    Log.i(ID, "Enabling Logging relay");
    Logging::enableCanbusRelay(); // Allow logging though canbus
#endif
    loadBuffers();
    loadStateMap();

    Log.i(ID, "Setting commands");
    Cmd::setCommand(COMMAND_ENABLE_CHARGING, setChargeSignal);
    Cmd::setCommand(COMMAND_SEND_CANBUS_MESSAGE, pushCanMessage);
    Cmd::setCommand(COMMAND_TOGGLE_CANBUS_SNIFF, toggleCanbusSniffer);
    Cmd::setCommand(COMMAND_SEND_ECHO, sendEchoMessage);
    Cmd::setCommand(COMMAND_TOGGLE_REVERSE, toggleMotorDirection);

    Heartbeat::beginReceiving();
#ifdef CONF_ECU_DEBUG
    Mirror::setup();
#endif

    static bool hasBeat = false;

    Log.d(ID, "Delaying 2 sec");
    Serial.flush();
    blinkStart();
    Pins::setInternalValue(PINS_INTERNAL_SYNC, 1);
    blinkStart();
    Serial.flush();
#ifndef TESTING
    while (!Heartbeat::checkBeat()) {
        delay(500);
    }
#endif

    while (true) {
#ifdef TESTING
        testValues();
#else
        if (timeElapsed >= 20) { // High priority updates
            timeElapsed = 0;
            Cmd::receiveCommand();
#ifndef SILENT
            Log(ID, "Current Motor Speed:", motorSpeed());
#endif
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
                on = Pins::getCanPinValue(PINS_INTERNAL_START);
            }
#ifndef SILENT
            Log(ID, "Start Light", on);
#endif
            Pins::setPinValue(PINS_FRONT_START_LIGHT, on);

            if (Fault::anyFault()) {
                Fault::logFault();
            }
        }
        if (timeElapsedLong >= 800) { // Low priority updates
            timeElapsedLong = 0;
            Pins::setPinValue(PINS_FRONT_BMS_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_BMS_FAULT));
            Pins::setPinValue(PINS_FRONT_IMD_LIGHT, Pins::getCanPinValue(PINS_INTERNAL_IMD_FAULT));

#ifndef SILENT
            // Motor controllers
            Log(ID, "MC0 DC BUS Voltage:", MC0Voltage());
            Log(ID, "MC1 DC BUS Voltage:", MC1Voltage());
            Log(ID, "MC0 DC BUS Current:", MC0Current());
            Log(ID, "MC1 DC BUS Current:", MC1Current());
            Log(ID, "MC0 Board Temp:", MC0BoardTemp());
            Log(ID, "MC1 Board Temp:", MC1BoardTemp());
            Log(ID, "MC0 Motor Temp:", MC0MotorTemp());
            Log(ID, "MC1 Motor Temp:", MC1MotorTemp());
            Log(ID, "MC Current Power:", MCPowerValue());

            // BMS
            Log(ID, "BMS State Of Charge:", BMSSOC());
            Log(ID, "BMS Immediate Voltage:", BMSVOLT());
            Log(ID, "BMS Pack Average Current:", BMSAMP());
            Log(ID, "BMS Pack Highest Temp:", BMSTempHigh());
            Log(ID, "BMS Pack Lowest Temp:", BMSTempLow());
            Log(ID, "BMS Discharge current limit:", BMSDischargeCurrentLimit());
            Log(ID, "BMS Charge current limit:", BMSChargeCurrentLimit());

            // General
            Log(ID, "Fault State", Pins::getCanPinValue(PINS_INTERNAL_GEN_FAULT));
#endif
        }
#endif
    }
}