#include "ECU.h"
#include "ECUGlobalConfig.h"
#include "Faults.h"
#include "Log.h"

#define INTERVAL_HIGH_PRIORITY 20
#define INTERVAL_MED_HIGH_PRIORITY 400
#define INTERVAL_MED_LOW_PRIORITY 800
#define INTERVAL_LOW_PRIORITY 1200

/**
 * @brief Name space used solely for front ECU logic
 */
namespace Front {

LOG_TAG ID = "Front Teensy";

extern struct State::State_t *currentState;

/**
 * @brief Runs front ECU code
 */
void run();

/**
 * @brief Load values that ID each unique state
 */
void loadStateMap();

/**
 * @brief Update and outputs the current state
 */
void updateCurrentState();

/**
 * @brief Enables the charging signal dependent on the current state
 */
void setChargeSignal();

/**
 * @brief Outputs values that are of lower priority
 */
void lowPriorityValues();

/**
 * @brief Outputs values that are of higher priority
 */
void highPriorityValues();
}