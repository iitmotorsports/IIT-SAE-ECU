#include <stdint.h>
#include <stdlib.h>

#include "Pins.h"

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

} // namespace Aero
