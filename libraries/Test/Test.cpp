/**
 * @file Test.cpp
 * @author IR
 * @brief Test source file
 * @version 0.1
 * @date 2022-01-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "WProgram.h"

// @cond

void serial_spam() {
    static elapsedMicros em;
    static double c = 5;
    int ramp = 5000;
    int ramp2 = 10;

    while (true) {
        Serial.write((char *)&ramp, 1);
        for (size_t i = 0; i < ramp; i++) {
            long long a = (long long)c;
            c = a / c;
        }
        if (--ramp2 <= 0 && ramp != 0) {
            ramp2 = 100;
            ramp--;
        }
    }
}

// @endcond