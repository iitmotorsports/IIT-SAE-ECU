#include "Front.h"

static LOG_TAG ID = "Front Teensy";

static elapsedMillis timeElapsed;
static uint8_t *MC0_RPM_Buffer;
static uint8_t *MC1_RPM_Buffer;
static uint8_t *MC0_VOLT_Buffer;
static uint8_t *MC1_VOLT_Buffer;
static uint8_t *MC0_CURR_Buffer;
static uint8_t *MC1_CURR_Buffer;
static uint8_t *BMS_SOC_Buffer;
static constexpr float wheelRadius = 1.8; // TODO: Get car wheel radius
static bool charging = false;

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

void Front::run() {
    Log.i(ID, "Teensy 3.6 SAE FRONT ECU Initalizing");
    Log.i(ID, "Setting up Canbus");
    Canbus::setup(); // allocate and organize addresses
    Log.i(ID, "Initalizing Pins");
    Pins::initialize(); // setup predefined pins
    // Fault::setup();               // load all buffers
    Log.i(ID, "Enabling Logging relay");
    Logging::enableCanbusRelay(); // Allow logging though canbus

    Log.i(ID, "Loading Buffers");
    MC0_RPM_Buffer = Canbus::getBuffer(ADD_MC0_RPM);
    MC1_RPM_Buffer = Canbus::getBuffer(ADD_MC1_RPM);
    MC0_VOLT_Buffer = Canbus::getBuffer(ADD_MC0_VOLT);
    MC1_VOLT_Buffer = Canbus::getBuffer(ADD_MC1_VOLT);
    MC0_CURR_Buffer = Canbus::getBuffer(ADD_MC0_CURR);
    MC1_CURR_Buffer = Canbus::getBuffer(ADD_MC1_CURR);
    BMS_SOC_Buffer = Canbus::getBuffer(ADD_BMS_SOC);

    Log.d(ID, "Delaying for debug");
    delay(10000);

    while (true) {
        if (timeElapsed >= 10) { // Update Tablet every 10ms
            timeElapsed = 0;

            Log.d(ID, "Checking Internal Pins");
            if (Pins::getCanPinValue(PINS_INTERNAL_START)) {
                Pins::setPinValue(PINS_FRONT_START_LIGHT, HIGH);
            } else {
                Pins::setPinValue(PINS_FRONT_START_LIGHT, LOW);
            }
            if (Pins::getCanPinValue(PINS_INTERNAL_BMS_FAULT)) {
                Pins::setPinValue(PINS_FRONT_BMS_LIGHT, HIGH);
            } else {
                Pins::setPinValue(PINS_FRONT_BMS_LIGHT, LOW);
            }
            if (Pins::getCanPinValue(PINS_INTERNAL_IMD_FAULT)) {
                Pins::setPinValue(PINS_FRONT_IMD_LIGHT, HIGH);
            } else {
                Pins::setPinValue(PINS_FRONT_IMD_LIGHT, LOW);
            }

            Log.d(ID, "Checking Tablet Serial");

            uint8_t serialData = 0;
            if (Serial.available()) {
                serialData = Serial.read();
                Log.d(ID, "Data received: ", serialData);
            }

            if (serialData == 123 /*&& Pins::getCanPinValue(PINS_INTERNAL_IDLE_STATE)*/) { // TODO: uncomment
                if (!charging) {
                    Pins::setInternalValue(PINS_INTERNAL_CHARGE_SIGNAL, HIGH);
                    Log.i(ID, "Charging Enabled");
                    charging = true;
                } else {
                    Pins::setInternalValue(PINS_INTERNAL_CHARGE_SIGNAL, LOW);
                    Log.i(ID, "Charging Stopped"); // TODO: use back pin PINS_BACK_CHARGING_RELAY to determine when charging stops as well
                    charging = false;
                }
            }

            Log.d(ID, "Updating Tablet data");

            // TODO: explicitly tell tablet about being in a fault state

            // receive rpm of MCs, interpret, then send to from teensy for logging
            Canbus::setSemaphore(ADD_MC0_RPM);
            int16_t MC_Rpm_Val_0 = *(int16_t *)(MC0_RPM_Buffer + 2); // Bytes 2-3 : Angular Velocity
            Canbus::setSemaphore(ADD_MC1_RPM);
            int16_t MC_Rpm_Val_1 = *(int16_t *)(MC1_RPM_Buffer + 2); // Bytes 2-3 : Angular Velocity
            Canbus::clearSemaphore();
            float MC_Spd_Val_0 = wheelRadius * 2 * 3.1415926536 / 60 * MC_Rpm_Val_0;
            float MC_Spd_Val_1 = wheelRadius * 2 * 3.1415926536 / 60 * MC_Rpm_Val_1;
            float speed = (MC_Spd_Val_0 + MC_Spd_Val_1) / 2;
            // Log.i(ID, "Current Motor Speed:", speed);
            // Log.i(ID, "Current Power Value:", powerValue());   // Canbus message from MCs
            // Log.i(ID, "BMS State Of Charge Value:", BMSSOC()); // Canbus message
        }
    }
}