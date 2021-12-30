#include "ECU.h"
#include "ECUGlobalConfig.h"
#include "Faults.h"
#include "Log.h"

#define INTERVAL_HIGH_PRIORITY 20
#define INTERVAL_MED_HIGH_PRIORITY 400
#define INTERVAL_MED_LOW_PRIORITY 800
#define INTERVAL_LOW_PRIORITY 1200

/**
 * @brief Name space used solely for front Teensy logic
 */
namespace Front {

LOG_TAG ID = "Front Teensy";

extern struct State::State_t *currentState;

/**
 * @brief Runs front Teensy code
 */
void run();
void loadStateMap();
void updateCurrentState();
void setChargeSignal();
void logValues();
}