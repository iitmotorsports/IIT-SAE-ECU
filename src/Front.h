#include "ECU.h"
#include "ECUGlobalConfig.h"
#include "Faults.h"
#include "Log.h"

#define COMMAND_ENABLE_CHARGING 123

/**
 * @brief Name space used solely for front Teensy logic
 * 
 */
namespace Front {
/**
 * @brief Runs front Teensy code
 * 
 */
void run();
}