/**
 * @file Faults.h
 * @author IR
 * @brief Checks for defined faults from canbus addresses or pins
 * @version 0.1
 * @date 2021-01-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "Canbus.h"

namespace Fault {

/**
 * @brief Setups canbus addresses to be used for fault checking
 * 
 */
void setup(void);

/**
 * @brief Checks if any serious fault has occurred
 * 
 * @return true A serious fault has been tripped
 * @return false No serious fault has been tripped
 */
bool hardFault(void);

/**
 * @brief Check if any non-serious fault has occurred
 * 
 * @return true A non-serious fault has been tripped
 * @return false No non-serious fault has been tripped
 */
bool softFault(void);

/**
 * @brief Checks both hardFault and softFault
 * 
 * @return true A fault has been tripped
 * @return false No fault has been tripped
 */
bool anyFault(void);

/**
 * @brief Interprets and logs the last fault that was checked using the Log library
 * @note Recommended not to run right after checking for hard faults, first attend to the hard fault
 */
void logFault(void);

} // namespace Fault