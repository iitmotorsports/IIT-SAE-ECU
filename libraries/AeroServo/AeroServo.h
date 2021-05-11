/**
 * @file AeroServo.h
 * @author IR
 * @brief Interpretation of Aero subteam code
 * @version 0.1
 * @date 2021-03-03
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef __ECU_AEROSERVO_H__
#define __ECU_AEROSERVO_H__

#include <stdint.h>
#include <stdlib.h>

#include "Pins.h"

/**
 * @brief Module used for aero calculations
 */
namespace Aero {

/**
 * @brief Attach analog pins to their servo objects
 * 
 */
void setup(void);

/**
 * @brief Run Aero servo logic, given raw values
 * 
 * @param breakPressure break pressure value
 * @param steeringAngle steering angle value
 */
void run(int breakPressure, int steeringAngle);

/**
 * @brief Get the current servo position value
 * 
 * @return int servo position value
 */
int getServoValue();

} // namespace Aero

#endif // __ECU_AEROSERVO_H__