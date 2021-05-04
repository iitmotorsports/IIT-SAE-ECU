#include "ECU.h"
#include "ECUGlobalConfig.h"
#if CONF_ECU_POSITION == FRONT_ECU
#include "Front.h"
#endif

// NOTE: Consider using back teensy as a dumb relay and have front teensy only be used for actual logic

int main(void) {
    Serial.begin(CONF_TEENSY_BAUD_RATE);
    delay(CONF_TEENSY_INITAL_DELAY);
    // pinMode(PINS_BACK_SHUTDOWN_SIGNAL, INPUT);
    // while(true){
    //     Serial.printf("%u %u\n",digitalRead(PINS_BACK_SHUTDOWN_SIGNAL),digitalReadFast(PINS_BACK_SHUTDOWN_SIGNAL));
    //     delay(50);
    // }
    // analogReadResolution(12);
    // int minVal0 = 7348957;
    // int maxVal0 = 0;
    // int minVal1 = 7348957;
    // int maxVal1 = 0;
    // int minVal2 = 7348957;
    // int maxVal2 = 0;
    
    // while (true) {
    //     int e = analogRead(16);
    //     int s = analogRead(18);
    //     int steer = analogRead(15);
    //     Serial.printf("%d %d dv:%d rat:%f\n", e, s, abs(e - s), ((float)e / (float)s));
    //     minVal2 = min(steer, minVal2);
    //     maxVal2 = max(steer, maxVal2);
    //     // Serial.printf("minsteer %d maxsteer %d\n", minVal2, maxVal2);
    //     minVal1 = min(e, minVal1);
    //     maxVal1 = max(e, maxVal1);
    //     // Serial.printf("minpedal0 %d maxpedal0 %d\n", minVal1, maxVal1);
    //     minVal0 = min(s, minVal0);
    //     maxVal0 = max(s, maxVal0);
    //     // Serial.printf("minpedal1 %d maxpedal1 %d\n", minVal0, maxVal0);
    //     // delay(250);
    // }
#if CONF_ECU_POSITION == BACK_ECU
    State::begin(ECUStates::Initialize_State);
#else
    Front::run();
#endif
    return 0;
}
