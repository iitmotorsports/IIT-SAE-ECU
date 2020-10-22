#ifndef __MCU_CANBUS_H__
#define __MCU_CANBUS_H__

#include <stdint.h>
#include <stdlib.h>

#include "CanbusConfig.def"
#include "FlexCAN_T4.h"
#include "WProgram.h"

namespace Canbus {

void setup(void);
void update(void);

} // namespace Canbus

#endif // __MCU_CANBUS_H__