#include "ECUStates.hpp"
#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "Faults.h"
#include "Heartbeat.h"
#include "Log.h"
#include "Mirror.h"
#include "MotorControl.def"
#include "MotorControl.h"
#include "Util.h"
#include "pump.h"

static bool FaultCheck() {
    if (Fault::hardFault() || Fault::softFault())
        return true;
    if (!Heartbeat::checkBeat()) // TODO: heartbeat front to back
        return true;
    return false;
};

// static void wait(long unsigned int millis) {
//     static elapsedMillis timeElapsed;
//     timeElapsed = 0;
//     while (timeElapsed <= millis)
//         ;
// }

void LEDBlink() {
    Pins::setPinValue(LED_BUILTIN, 0);
    delay(100);
    Pins::setPinValue(LED_BUILTIN, 1);
    delay(100);
    Pins::setPinValue(LED_BUILTIN, 0);
    delay(100);
    Pins::setPinValue(LED_BUILTIN, 1);
    delay(100);
    Pins::setPinValue(LED_BUILTIN, 0);
}

static void updateFaultLights() {
    static int bms, imd, bms_l, imd_l = 0;
    if ((bms = Pins::getPinValue(PINS_BACK_BMS_FAULT)) != bms_l || (imd = Pins::getPinValue(PINS_BACK_IMD_FAULT)) != imd_l) {
#ifdef CONF_ECU_DEBUG
        Log.d("FaultLights", "Updating Lights");
#endif
        Pins::setInternalValue(PINS_INTERNAL_BMS_FAULT, bms);
        Pins::setInternalValue(PINS_INTERNAL_IMD_FAULT, imd);
        bms_l = bms;
        imd_l = imd;
    }
    int breakVal = Pins::getCanPinValue(PINS_FRONT_BRAKE);
    Pins::setPinValue(PINS_BACK_BRAKE_LIGHT, PINS_ANALOG_HIGH * (breakVal > (CONF_BRAKE_MIN + 32)));
}

State::State_t *ECUStates::Initialize_State::run(void) {
    Log.i(ID, "Teensy 4.1 SAE BACK ECU Initalizing");
    Log.i(ID, "Setup canbus");
    Canbus::setup(); // allocate and organize addresses
    Log.i(ID, "Initialize pins");
    Pins::initialize(); // setup predefined pins
    LEDBlink();
    Log.i(ID, "Waiting for sync");
    while (!Pins::getCanPinValue(PINS_INTERNAL_SYNC)) {
        Canbus::sendData(ADD_MC0_CTRL);
        Canbus::sendData(ADD_MC1_CTRL);
        delay(100);
    }
    MC::setup();
#ifdef CONF_ECU_DEBUG
    Mirror::setup();
#endif
    Heartbeat::addCallback(updateFaultLights); // IMPROVE: don't just periodically check if leds are on
    Heartbeat::beginBeating();
    Heartbeat::beginReceiving();
    Pump::start();
    // Pump::set(100);

    delay(500);

    if (getLastState() != &ECUStates::FaultState) {
        Log.d(ID, "Waiting for initial fault reset");
        Pins::setPinValue(PINS_BACK_ECU_FAULT, LOW);
        while (Pins::getPinValue(PINS_BACK_FAULT_RESET)) {
            Log.d(ID, "FAULT BTN VAL", Pins::getPinValue(PINS_BACK_FAULT_RESET), 1);
            Logging::trySDMode();
        }
        Log.d(ID, "FAULT BTN VAL", Pins::getPinValue(PINS_BACK_FAULT_RESET), 1);
        Pins::setPinValue(PINS_BACK_ECU_FAULT, HIGH);
    }

    // Front teensy should know when to blink start light
    // Log.d(ID, "Checking for Inital fault");

    // NOTE: IMD Fault does not matter in initalizing state
    // if (!Pins::getPinValue(PINS_BACK_IMD_FAULT) && FaultCheck()) {
    //     Log.e(ID, "Inital fault check tripped");
    //     return &ECUStates::FaultState;
    // }
    Log.i(ID, "Waiting for clear faults");
    while (true) {
        if (!FaultCheck())
            break;
        delay(500);
    }

    // TSV
    Log.i(ID, "Waiting for shutdown signal");

    elapsedMillis shutdownBounce;

    while (true) {
        if (!Pins::getPinValue(PINS_BACK_SHUTDOWN_SIGNAL)) {
            if (shutdownBounce > 50)
                break;
        } else {
            shutdownBounce = 0;
        }
    }

    Log.i(ID, "Shutdown signal received");

    Log.d(ID, "Finshed Setup");
    return &ECUStates::PreCharge_State;
};

State::State_t *ECUStates::PreCharge_State::PreCharFault(void) {
    Log.w(ID, "Opening Air2 and Precharge Relay");
    Pins::setPinValue(PINS_BACK_AIR2, LOW);
    Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, LOW);
    Log.e(ID, "Precharge Faulted during precharge");
    return &ECUStates::FaultState;
};

bool ECUStates::PreCharge_State::voltageCheck(bool *fault) {
    Canbus::Buffer::lock BMS_lock = BMS_DATA_Buffer->get_lock(Canbus::DEFAULT_TIMEOUT);
    Canbus::Buffer::lock MC0_lock = MC0_VOLT_Buffer->get_lock(Canbus::DEFAULT_TIMEOUT);
    Canbus::Buffer::lock MC1_lock = MC1_VOLT_Buffer->get_lock(Canbus::DEFAULT_TIMEOUT);

    if (!BMS_lock.locked || !MC0_lock.locked || !MC1_lock.locked) {
        *fault = true;
        return false;
    }

    int16_t BMSVolt = BMS_DATA_Buffer->getShort(2) / 10; // Byte 2-3: Pack Instant Voltage
    int16_t MC0Volt = MC0_VOLT_Buffer->getShort(0) / 10; // Bytes 0-1: DC BUS MC Voltage
    int16_t MC1Volt = MC1_VOLT_Buffer->getShort(0) / 10; // Bytes 0-1: DC BUS MC Voltage

    Log.d(ID, "BMS Voltage", BMSVolt, -250);
    Log.d(ID, "BMS MC0 Voltage", MC0Volt, -250);
    Log.d(ID, "BMS MC1 Voltage", MC1Volt, -250);

    return 0.91 * BMSVolt <= (MC0Volt + MC1Volt) / 2;
}

State::State_t *ECUStates::PreCharge_State::run(void) { // NOTE: Low = Closed, High = Open
    Log.i(ID, "Precharge running");

    elapsedMillis timeElapsed;

    bool fault = false;

    // Log.i(ID, "Ensuring voltage check");
    // while (!voltageCheck(&fault)) {
    //     delay(100);
    // }

    if (FaultCheck() || fault) {
        Log.e(ID, "Precharge Faulted before precharge");
        return &ECUStates::FaultState;
    }

    Log.w(ID, "Opening Air2 and closing precharge");
    Pins::setPinValue(PINS_BACK_AIR2, LOW);
    Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, HIGH);

    if (FaultCheck()) {
        return PreCharFault();
    }

    Log.d(ID, "Running precharge loop");

    while (timeElapsed <= 12000) {
        // FIXME: will always pass if submodules are disconnected from CAN net
        if (voltageCheck(&fault) && !(fault |= FaultCheck()) && timeElapsed >= 5000) {
            Log.i(ID, "Precharge Finished, closing Air2");
            Pins::setPinValue(PINS_BACK_AIR2, HIGH);
            delay(650);
            Log.w(ID, "Opening precharge relay");
            Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, LOW);
            return &ECUStates::Idle_State;
        } else if (fault) {
            Log.e(ID, "Precharge faulted");
            return PreCharFault();
        }
    }

    Log.e(ID, "Precharge timed out");
    return PreCharFault();
};

State::State_t *ECUStates::Idle_State::run(void) {
    Log.i(ID, "Waiting for Button not to be pressed");
    while (!Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT_OFF)) {
    }

    Log.i(ID, "Waiting for Button or Charging Press");

    // Front teensy should already be blinking start light

    elapsedMillis waiting;

    while (true) {
        if (!Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT_OFF)) {
            Log.i(ID, "Button Pressed");
            // Front teensy will stop blinking start light
            return &ECUStates::Button_State;
        } else if (Pins::getCanPinValue(PINS_INTERNAL_CHARGE_SIGNAL)) {
            Log.i(ID, "Charging Pressed");
            // Front teensy will continue blinking start light in charge state
            return &ECUStates::Charging_State;
        } else if (FaultCheck()) { // TODO: rate limit logging
            Log.w(ID, "Fault in idle state");
            break;
        }
    }
    return &ECUStates::FaultState;
}

State::State_t *ECUStates::Charging_State::run(void) {
    Pins::setPinValue(PINS_BACK_CHARGING_RELAY, HIGH);
    Log.i(ID, "Charging on");

    elapsedMillis voltLogNotify;

    while (Pins::getCanPinValue(PINS_INTERNAL_CHARGE_SIGNAL)) {
        // IMPROVE: Don't use fault to stop charging
        if (FaultCheck()) {
            Pins::setPinValue(PINS_BACK_CHARGING_RELAY, LOW);
            Log.e(ID, "Charging faulted, turning off");
            return &ECUStates::FaultState;
        }
    }

    Pins::setPinValue(PINS_BACK_CHARGING_RELAY, LOW);
    Log.i(ID, "Charging turning off");

    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::Button_State::run(void) {
    Log.i(ID, "Waiting for Button not to be pressed");
    while (!Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT_OFF)) {
    }
    Log.i(ID, "Playing sound");

    Pins::setPinValue(PINS_BACK_SOUND_DRIVER, PINS_ANALOG_HIGH);

    elapsedMillis soundTimer;

    while (soundTimer < 3000) {
        if (FaultCheck()) {
            Log.e(ID, "Failed to play sound");
            Pins::setPinValue(PINS_BACK_SOUND_DRIVER, LOW);
            return &ECUStates::FaultState;
        }
    }

    Pins::setPinValue(PINS_BACK_SOUND_DRIVER, LOW);
    Log.i(ID, "Playing sound finished");

    return &ECUStates::Driving_Mode_State;
}

void ECUStates::Driving_Mode_State::carCooling(bool enable) { // NOTE: Cooling values are all static
    int setOn = enable * PINS_ANALOG_MAX;
    Pump::set(enable * 100);
    Pins::setPinValue(PINS_BACK_FAN1_PWM, setOn);
    Pins::setPinValue(PINS_BACK_FAN2_PWM, setOn);
};

State::State_t *ECUStates::Driving_Mode_State::DrivingModeFault(void) {
    Log.i(ID, "Fault happened in driving state");
    Pins::setInternalValue(PINS_INTERNAL_START, false);
    carCooling(false);
    Log.i(ID, "Starting MC heartbeat");
    MC::enableMotorBeating(true);
    return &ECUStates::FaultState;
}

// NOTE: MCs weak faults also cause a true fault here
State::State_t *ECUStates::Driving_Mode_State::run(void) {
    Log.i(ID, "Driving mode on");
    Log.i(ID, "Cooling on");
    carCooling(true);

    elapsedMillis controlDelay;

    Log.i(ID, "Stopping MC heartbeat");
    MC::enableMotorBeating(false);

    Log.d(ID, "Sending Fault reset to MCs complete");
    MC::clearFaults(); // Clear fault if any

    Pins::setInternalValue(PINS_INTERNAL_START, true);

    Log.d(ID, "Entering drive loop");
    while (true) {
        if (controlDelay > 20) { // NOTE: Each data frame is 89 bits long thus at 250kbps the MC buses can handle a maximum of 2808 messages per second
            controlDelay = 0;

            static bool runReverse = false;

            if (Pins::getCanPinValue(PINS_INTERNAL_REVERSE) != runReverse) {
                runReverse = Pins::getCanPinValue(PINS_INTERNAL_REVERSE);
                MC::setDirection(!runReverse);
            }

            if (Fault::anyFault()) { // FIXME: are motor controller faults actually being picked up?
                return DrivingModeFault();
            }

            // #if ECU_TESTING == BACK_ECU
            if (((MC0_VOLT_Buffer->getShort(0) / 10) + (MC1_VOLT_Buffer->getShort(0) / 10)) / 2 < 90) { // 'HVD Fault'
                Log.e(ID, "'HVD Fault' MC voltage < 90");
                return DrivingModeFault();
            }
            // #endif

            int breakVal = Pins::getCanPinValue(PINS_FRONT_BRAKE);
            int steerVal = Pins::getCanPinValue(PINS_FRONT_STEER);

            int pedal0 = Pins::getCanPinValue(PINS_FRONT_PEDAL0);
            int pedal1 = Pins::getCanPinValue(PINS_FRONT_PEDAL1);

            int pAVG = (pedal0 + pedal1) / 2;

            // NOTE: pedal has a threshold value
            if (pAVG >= 100 && (float)abs(pedal1 - pedal0) / PINS_ANALOG_HIGH > 0.2f) {
                Log.e(ID, "Pedal value offset > 10%");
                return DrivingModeFault();
            }

            // TODO: passive regen braking, only for hawkrod

            // if (pAVG >= 200) {
            MC::setTorque(pAVG, breakVal, steerVal);
            // } else if (MC::motorSpeed() > 25) { // FIXME: Seems to just disable motors?
            //     MC::sendTorque(ADD_MC0_CTRL, -160, 1, 1);
            //     MC::sendTorque(ADD_MC1_CTRL, -160, 1, 1);
            // }

            if (Fault::softFault()) {
                Fault::logFault();
            }

            Log.i(ID, "Last MC0 Torque Value:", MC::getLastTorqueValue(true), true);
            Log.i(ID, "Last MC1 Torque Value:", MC::getLastTorqueValue(false), true);
        }

        if (!Pins::getCanPinValue(PINS_FRONT_BUTTON_INPUT_OFF)) {
            Log.w(ID, "Going back to Idle state");
            break;
        }
    }

    Pins::setInternalValue(PINS_INTERNAL_START, false);

    Log.i(ID, "Starting MC heartbeat");
    MC::enableMotorBeating(true);

    carCooling(false);

    Log.i(ID, "Driving mode off");
    return &ECUStates::Idle_State;
}

State::State_t *ECUStates::FaultState::run(void) {
    Pins::setInternalValue(PINS_INTERNAL_GEN_FAULT, 1);
    Canbus::enableInterrupts(false);

    Log.w(ID, "Opening Air1, Air2 and Precharge Relay");
    Pins::setPinValue(PINS_BACK_AIR2, LOW);
    Pins::setPinValue(PINS_BACK_PRECHARGE_RELAY, LOW);

    Log.w(ID, "Resetting pins");
    Pins::resetPhysicalPins();

    if (getLastState() == &ECUStates::PreCharge_State) {
        while (true) {
            Log.e(ID, "Precharge fault");
            Pins::setInternalValue(PINS_INTERNAL_GEN_FAULT, 1);
            Fault::logFault();
            Fault::anyFault();
            delay(1000);
        }
    } else {
        Log.e(ID, "FAULT STATE");
        Fault::logFault();
        delay(500);
    }
    Pins::setInternalValue(PINS_INTERNAL_GEN_FAULT, 0);
    return &ECUStates::Initialize_State;
}