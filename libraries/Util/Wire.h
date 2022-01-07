/**
 * @file Wire.h
 * @author IR
 * @brief Convenience header for alternative libs to the Arduino Wire.h library
 * @version 0.1
 * @date 2022-01-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __WIRE_H__
#define __WIRE_H__

#ifdef __MK66FX1M0__
#include "i2c_t3.h"
using TwoWire = i2c_t3;
#endif

#ifdef __IMXRT1062__
#include "i2c_driver_wire.h"
#endif
#endif // __WIRE_H__