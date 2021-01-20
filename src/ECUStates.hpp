#include <stdint.h>
#include <stdlib.h>

#include "WProgram.h"

#include "Canbus.h"
#include "Log.h"
#include "Pins.h"
#include "State.h"

/**
 * @brief The Teensy specific state declarations
 */
namespace ECUStates {

/**
 * @brief Teensy initial state
 */
static struct Initialize : State::State_t {
    LOG_TAG ID = "Teensy Initialize";
    State::State_t *run(void);
} Initialize;

/**
 * @brief Teensy PreCharge State. Steps though the precharge circuit, waiting when necessary.
 */
static struct PreCharge_State : State::State_t {
private:
    static const uint8_t Air1_Pin = 13;            // TODO: Air1_Pin out
    static const uint8_t Air2_Pin = 13;            // TODO: Air2_Pin out
    static const uint8_t Precharge_Relay_Pin = 13; // TODO: Precharge_Relay_Pin out
    static const uint8_t BMS_Voltage_Pin = 37;     // TODO: BMS_Voltage_Pin in
    static const uint8_t MC_Voltage_Pin = 37;      // TODO: MC_Voltage_Pin in
    State::State_t *PreCharFault();
    bool checkPreFault();

public:
    LOG_TAG ID = "PreCharge State";
    State::State_t *run(void);
} PreCharge_State;

/**
 * @brief Idle state has buttonpressed, charging state, 
 * and fault booleans with input time for this state
 * 
 */
static struct Idle_State : State::State_t {
private:
    static const uint8_t Button_Input_Pin = 37;   // TODO: Button input
    static const uint8_t Charging_Input_Pin = 37; // TODO: Charging input

public:
    LOG_TAG ID = "Idle State";
    State::State_t *run(void);
} Idle_State;

/**
 * @brief charging state has booleans of relay charge, charging on, fault, and voltage data as a string
 */
static struct Charging_State : State::State_t {
private:
    static const uint8_t Charging_Relay_Pin = 13;   // TODO: Charging Relay out
    static const uint8_t Charging_Signal_Pin = 37;  // TODO: Charging Signal in
    static const uint8_t Charging_Voltage_Pin = 37; // TODO: Charging Volt in
    bool checkChargingFault();

public:
    LOG_TAG ID = "Charging State";
    bool ChargingOn;
    bool fault = false;
    String Voltage_Data = " ";

    State::State_t *run(void);

} Charging_State;

/**
 * @brief Button state has playing sound, ready signal. and fault booleans 
 * for this state (input time acknowledged)
 */
static struct Button_State : State::State_t { // NOTE: Is button state just play sound state?
    LOG_TAG ID = "Button State";
    State::State_t *run(void);

} Button_State;

/**
 * @brief driving mode state has interrupt, driving mode, safe to continue, message incoming, fault, 
 * info of message, and message handler attributes for this state. 
 * 
 */
static struct Driving_Mode_State : State::State_t {
private:
    elapsedMillis PollingTime;
    bool Interrupts = false;
    bool Driving_mode = false;
    bool SafeToContinue = false;

public:
    LOG_TAG ID = "Driving Mode State";
    bool MessageIncoming = false;
    bool Fault = false;
    String info = " ";
    String Message_Handler = " ";

    State::State_t *run(void);

} Driving_Mode_State;

/**
 * @brief Demonstrate Serial logging
 * This state demonstrates the diffrent ways we can log things to serial
 */
static struct Fault : State::State_t {
    LOG_TAG ID = "Fault State";
    State::State_t *run(void);
} Fault;

/**
 * @brief Demonstrate Serial logging
 * This state demonstrates the diffrent ways we can log things to serial
 */
static struct Logger_t : State::State_t {
    LOG_TAG ID = "Logger";
    State::State_t *run(void);
} Logger;

/**
 * @brief Demonstrate how to go back to a previous state
 */
static struct Bounce_t : State::State_t {
    LOG_TAG ID = "Bouncer";
    State::State_t *run(void);
} Bounce;

} // namespace ECUStates