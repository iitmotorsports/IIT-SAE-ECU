/**
 * @file DebugPanel.h
 * @author IR
 * @brief Debugging library, specific to our situation
 * @version 0.1
 * @date 2021-03-16
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ECU_DEBUGPANEL_H__
#define __ECU_DEBUGPANEL_H__

#include "Canbus.h"
#include "Log.h"
#include "Pins.h"
#include <stdint.h>
#include <stdlib.h>

namespace Debug {

void printPinMap();

void canBusSniffer(bool enable);

void sendCanBusMessage();

} // namespace Debug

#endif // __ECU_DEBUGPANEL_H__