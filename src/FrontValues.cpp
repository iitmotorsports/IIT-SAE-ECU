#include "Front.h"
#include "Util.h"
#include "MotorControl.h"

namespace Front {

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
    Log(ID, "Current State", currState, INTERVAL_MED_LOW_PRIORITY);
    currentState = stateMap[currState]; // returns NULL if not found
}

void setChargeSignal() {
    Pins::setInternalValue(PINS_INTERNAL_CHARGE_SIGNAL, currentState == &ECUStates::Idle_State);
}

void lowPriorityValues() {
    // Motor controllers
    Log(ID, "MC0 DC BUS Voltage:", MC0Voltage(), INTERVAL_LOW_PRIORITY);
    Log(ID, "MC1 DC BUS Voltage:", MC1Voltage(), INTERVAL_LOW_PRIORITY);
    Log(ID, "MC0 DC BUS Current:", MC0Current(), INTERVAL_LOW_PRIORITY);
    Log(ID, "MC1 DC BUS Current:", MC1Current(), INTERVAL_LOW_PRIORITY);
    Log(ID, "MC0 Board Temp:", MC0BoardTemp(), INTERVAL_LOW_PRIORITY);
    Log(ID, "MC1 Board Temp:", MC1BoardTemp(), INTERVAL_LOW_PRIORITY);
    Log(ID, "MC0 Motor Temp:", MC0MotorTemp(), INTERVAL_LOW_PRIORITY);
    Log(ID, "MC1 Motor Temp:", MC1MotorTemp(), INTERVAL_LOW_PRIORITY);
    Log(ID, "MC Current Power:", MCPowerValue(), INTERVAL_LOW_PRIORITY);

    // BMS
    Log(ID, "BMS State Of Charge:", BMSSOC(), INTERVAL_LOW_PRIORITY);
    Log(ID, "BMS Immediate Voltage:", BMSVOLT(), INTERVAL_LOW_PRIORITY);
    Log(ID, "BMS Pack Average Current:", BMSAMP(), INTERVAL_LOW_PRIORITY);
    Log(ID, "BMS Pack Highest Temp:", BMSTempHigh(), INTERVAL_LOW_PRIORITY);
    Log(ID, "BMS Pack Lowest Temp:", BMSTempLow(), INTERVAL_LOW_PRIORITY);
    Log(ID, "BMS Discharge current limit:", BMSDischargeCurrentLimit(), INTERVAL_LOW_PRIORITY);
    Log(ID, "BMS Charge current limit:", BMSChargeCurrentLimit(), INTERVAL_LOW_PRIORITY);

    // General
    Log(ID, "Fault State", Pins::getCanPinValue(PINS_INTERNAL_GEN_FAULT), INTERVAL_LOW_PRIORITY);
}

static double lastBrake = 0.0, lastSteer = 0.0, lastPedal0 = 0.0, lastPedal1 = 0.0;

void highPriorityValues() {
    Log(ID, "Current Motor Speed:", MC::motorSpeed(), true);
    Log.d(ID, "Motor 0 Speed", MC::motorSpeed(0), true);
    Log.d(ID, "Motor 1 Speed", MC::motorSpeed(1), true);

    int pedal0, pedal1;
    Log.i(ID, "Brake", lastBrake = EMAvg(lastBrake, Pins::getPinValue(PINS_FRONT_BRAKE), 4), true);
    Log.i(ID, "Steering", lastSteer = EMAvg(lastSteer, Pins::getPinValue(PINS_FRONT_STEER), 4), true);
    Log.i(ID, "Pedal 0", (pedal0 = (lastPedal0 = EMAvg(lastPedal0, Pins::getPinValue(PINS_FRONT_PEDAL0), 4))), true);
    Log.i(ID, "Pedal 1", (pedal1 = (lastPedal1 = EMAvg(lastPedal1, Pins::getPinValue(PINS_FRONT_PEDAL1), 4))), true);
    Log.d(ID, "Pedal AVG", (pedal0 + pedal1) / 2, true);
}

} // namespace Front