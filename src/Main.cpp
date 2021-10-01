#include <stdint.h>
#include <stdlib.h>

#include "FlexCAN_T4.h"
#include "WProgram.h"

#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "Log.h"
#include "Pins.h"
#include "State.h"

int main(void) {
    Serial.begin(CONF_ECU_BAUD_RATE);
    delay(CONF_ECU_INITAL_DELAY);
#if CONF_ECU_POSITION == BACK_ECU
    Log("BACK TEENSY", "WASSUP 😎");
#else
    Log("FRONT TEENSY", "Oh Hi!");
#endif
    return 0;
}
