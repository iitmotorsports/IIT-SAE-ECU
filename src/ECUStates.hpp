#include <stdint.h>
#include <stdlib.h>

#include "FlexCAN_T4.h"
#include "WProgram.h"

#include "Pins.h"
#include "State.h"

/**
 * @brief The Teensy specific state declarations
 */
namespace ECUStates {

/**
 * @brief Teensy initial state
 */
static struct Initialize_t : State::State_t {
private:
    bool firstSetup;

public:
    LOG_TAG ID = "Teensy Initialize";
    State::State_t *run(void);
} Initialize;

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