#pragma once

#include "Wire.h"

namespace Pump {

static const int PUMP_I2C_ADDR = 0b1001;
// static const int PUMP_I2C_ADDR = 0b1000100;

void start();
/**
 * @brief Set pump percentage 0%-100%
 *
 * @param val
 */
void set(uint8_t val);

} // namespace Pump