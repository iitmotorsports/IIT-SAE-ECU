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
namespace ECUStates
{

    /**
 * @brief Teensy initial state
 */
    static struct Initialize : State::State_t
    {
        LOG_TAG ID = "Teensy Initialize";
        State::State_t *run(void);
    } Initialize;

    /**
 * @brief Teensy PreCharge State. Steps though the precharge circuit, waiting when necessary.
 */
    static struct PreCharge_State : State::State_t
    {
    private:
        uint8_t *BMS_Voltage_Buffer;
        uint8_t *MC0_Voltage_Buffer;
        uint8_t *MC1_Voltage_Buffer;
        void getBuffers();
        State::State_t *PreCharFault();
        bool voltageCheck();

    public:
        LOG_TAG ID = "PreCharge State";
        State::State_t *run(void);
    } PreCharge_State;

    /**
 * @brief Idle state has buttonpressed, charging state, 
 * and fault booleans with input time for this state
 * 
 */
    static struct Idle_State : State::State_t
    {
        LOG_TAG ID = "Idle State";
        State::State_t *run(void);
    } Idle_State;

    /**
 * @brief charging state has booleans of relay charge, charging on, fault, and voltage data as a string
 */
    static struct Charging_State : State::State_t
    {
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
    static struct Button_State : State::State_t
    { // NOTE: Is button state just play sound state?
        LOG_TAG ID = "Button State";
        State::State_t *run(void);

    } Button_State;

    /**
 * @brief driving mode state has interrupt, driving mode, safe to continue, message incoming, fault, 
 * info of message, and message handler attributes for this state. 
 * 
 */
    static struct Driving_Mode_State : State::State_t
    {
    private:
        static const float wheelRadius = 1.8; // TODO: Get car wheel radius
        uint8_t *MC0_RPM_Buffer;
        uint8_t *MC1_RPM_Buffer;
        uint8_t *MC0_PWR_Buffer;
        uint8_t *MC1_PWR_Buffer;
        uint8_t *BMS_SOC_Buffer;
        void getBuffers();
        void sendMCCommand(uint32_t MC_ADD, int torque, bool direction, bool enableBit);
        void torqueVector(int torques[2]);
        uint32_t BMSSOC();
        uint32_t powerValue();

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
    static struct FaultState : State::State_t
    {
        LOG_TAG ID = "Fault State";
        State::State_t *run(void);
    } FaultState;

    /**
 * @brief Demonstrate Serial logging
 * This state demonstrates the diffrent ways we can log things to serial
 */
    static struct Logger_t : State::State_t
    {
        LOG_TAG ID = "Logger";
        State::State_t *run(void);
    } Logger;

    /**
 * @brief Demonstrate how to go back to a previous state
 */
    static struct Bounce_t : State::State_t
    {
        LOG_TAG ID = "Bouncer";
        State::State_t *run(void);
    } Bounce;

} // namespace ECUStates