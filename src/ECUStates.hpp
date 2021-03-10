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
 * State which initializes everything
 */
static struct Initialize : State::State_t {
    LOG_TAG ID = "Teensy Initialize";
    State::State_t *run(void);
} Initialize;

/**
 * @brief Teensy PreCharge State
 * State which goes through the precharge circuit
 */
static struct PreCharge_State : State::State_t {
private:
    uint8_t *BMS_VOLT_Buffer;
    uint8_t *MC0_VOLT_Buffer;
    uint8_t *MC1_VOLT_Buffer;
    void getBuffers();
    State::State_t *PreCharFault();
    bool voltageCheck();

public:
    LOG_TAG ID = "PreCharge State";
    State::State_t *run(void);
} PreCharge_State;

/**
 * @brief Idle state
 * @details State that waits for input to go to Button or Charging state
 * 
 */
static struct Idle_State : State::State_t {
    LOG_TAG ID = "Idle State";
    State::State_t *run(void);
} Idle_State;

/**
 * @brief Charging state
 * @details State for charging, stops either when signal is received from android or charging is done
 */
static struct Charging_State : State::State_t {
    LOG_TAG ID = "Charging State";
    bool ChargingOn;
    bool fault = false;
    String Voltage_Data = " ";

    State::State_t *run(void);

} Charging_State;

/**
 * @brief Button state 
 * @details State where the speaker is played and enables driving mode
 */
static struct Button_State : State::State_t {
    LOG_TAG ID = "Button State";
    State::State_t *run(void);

} Button_State;

/**
 * @brief Driving mode state 
 * @details Main state that will run the car
 * 
 */
static struct Driving_Mode_State : State::State_t {
private:
    void sendMCCommand(uint32_t MC_ADD, int torque, bool direction, bool enableBit);
    void torqueVector(int torques[2], float pedalVal);
    void carCooling(float temp);

public:
    LOG_TAG ID = "Driving Mode State";
    bool MessageIncoming = false;
    bool Fault = false;
    String info = " ";
    String Message_Handler = " ";

    State::State_t *run(void);

} Driving_Mode_State;

/**
 * @brief Fault state
 * @details This state demonstrates the diffrent ways we can log things to serial
 */
static struct FaultState : State::State_t {
    LOG_TAG ID = "Fault State";
    State::State_t *run(void);
} FaultState;

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