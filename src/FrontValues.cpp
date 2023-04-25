#include "Front.h"
#include "MotorControl.h"
#include "Util.h"
#include "map"
//
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
    Log.p("state", "Current State", currState, INTERVAL_MED_LOW_PRIORITY);
    currentState = stateMap[currState]; // returns NULL if not found
}

void updateStartLight(bool hasBeat) {
    static bool on = false;
    if (hasBeat && (currentState == &ECUStates::Idle_State)) {
        on = !on;
    } else {
        on = Pins::getCanPinValue(PINS_INTERNAL_START);
    }
    Log("start_light", "Start Light", on, true);
    Pins::setPinValue(PINS_FRONT_START_LIGHT, on);
}

void setChargeSignal() {
    Pins::setInternalValue(PINS_INTERNAL_CHARGE_SIGNAL, currentState == &ECUStates::Idle_State);
}

void USBHostPush(const int id, const int value) {
    static std::map<int, int> update;
    if (update.find(id) == update.end()) {
        update[id] = value;
    }
    if (update[id] != value) {
        Serial.write((uint8_t *)&id, 4);
        Serial.write((uint8_t *)&value, 4);
    }
}

void lowPriorityValues() {
    // Motor controllers
    USBHostPush(4, MC0Voltage());    // MC0 DC BUS Voltage
    USBHostPush(5, MC1Voltage());    // MC1 DC BUS Voltage
    USBHostPush(7, MC0Current());    // MC0 DC BUS Current
    USBHostPush(6, MC1Current());    // MC1 DC BUS Current
    USBHostPush(9, MC0BoardTemp());  // MC0 Board Temp
    USBHostPush(8, MC1BoardTemp());  // MC1 Board Temp
    USBHostPush(11, MC0MotorTemp()); // MC0 Motor Temp
    USBHostPush(10, MC1MotorTemp()); // MC1 Motor Temp
    USBHostPush(13, MCPowerValue()); // MC Current Power
    // BMS
    USBHostPush(14, BMSSOC()); // BMS State Of Charge
    USBHostPush(15, BMSVOLT());                  // "BMS Immediate Voltage
    USBHostPush(16, BMSAMP());                   // BMS Pack Average Current
    USBHostPush(17, BMSTempHigh());              // BMS Pack Highest Temp
    USBHostPush(18, BMSTempLow());               // BMS Pack Lowest Temp
    USBHostPush(19, BMSDischargeCurrentLimit()); // BMS Discharge current limit
    USBHostPush(20, BMSChargeCurrentLimit());    // BMS Charge current limit

    // General
    Log.p("fault", "Fault State", Pins::getCanPinValue(PINS_INTERNAL_GEN_FAULT), INTERVAL_LOW_PRIORITY);
}

static double lastBrake = 0.0, lastSteer = 0.0, lastPedal0 = 0.0, lastPedal1 = 0.0;

void medPriorityValues() {
    int pedal0, pedal1;
    USBHostPush(1, (lastBrake = EMAvg(lastBrake, Pins::getPinValue(PINS_FRONT_BRAKE), 4)));               // Brake
    USBHostPush(27, (lastSteer = EMAvg(lastSteer, Pins::getPinValue(PINS_FRONT_STEER), 4)));              // Steering
    USBHostPush(2, (pedal0 = (lastPedal0 = EMAvg(lastPedal0, Pins::getPinValue(PINS_FRONT_PEDAL0), 4)))); // Pedal 0
    USBHostPush(3, (pedal1 = (lastPedal1 = EMAvg(lastPedal1, Pins::getPinValue(PINS_FRONT_PEDAL1), 4)))); // Pedal 1
    // pushToPhoneOverUsbSerialButOnHostWhenItIsDone(1, (pedal0 + pedal1) / 2);                                                                // Pedal AVG
}

void highPriorityValues() {
    USBHostPush(12, MC::motorSpeed()); // Current Motor Speed
    // pushToPhoneOverUsbSerialButOnHostWhenItIsDone(1, MC::motorSpeed(0)); // Motor 0 Speed
    // pushToPhoneOverUsbSerialButOnHostWhenItIsDone(1, MC::motorSpeed(1)); // Motor 1 Speed
}

} // namespace Front