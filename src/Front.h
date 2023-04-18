#ifndef __FRONT_H__
#define __FRONT_H__

#include "ECU.h"
#include "ECUGlobalConfig.h"
#include "Faults.h"
#include "Log.h"

#define INTERVAL_HIGH_PRIORITY 500
#define INTERVAL_MED_HIGH_PRIORITY 1200
#define INTERVAL_MED_LOW_PRIORITY 5000
#define INTERVAL_LOW_PRIORITY 1200

/**
 * @brief Name space used solely for front ECU logic
 */
namespace Front {

extern LOG_TAG ID;
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
 * @brief Update the startlight
 * 
 * @param hasBeat Whether the heartbeat is working
 */
void updateStartLight(bool hasBeat);

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
#endif // __FRONT_H__