#ifdef __MK66FX1M0__
#include "i2c_t3.h"
using TwoWire = i2c_t3;
#endif

#ifdef __IMXRT1062__
#include "i2c_driver_wire.h"
#endif