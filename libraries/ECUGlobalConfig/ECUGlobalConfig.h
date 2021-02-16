#ifndef __ECU_GLOBALCONFIG_H__
#define __ECU_GLOBALCONFIG_H__

#define CONF_ECU_DEBUG // Define to enable normal logging on back teensy and checks to be run throughout libaries

#define FRONT_ECU 1
#define BACK_ECU 0

#define CONF_ECU_POSITION BACK_ECU // Back Teensy, Used for documentation
// #define CONF_ECU_POSITION FRONT_ECU // Front Teensy

#define CONF_TEENSY_BAUD_RATE 115200
#define CONF_TEENSY_INITAL_DELAY 2000

#endif // __ECU_GLOBALCONFIG_H__