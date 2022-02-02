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
    static elapsedMillis em;
    long long ramp = 50000;

    while (true) {
        if (ramp < 0)
            continue;
        Serial.write((char *)&ramp, 8);
        for (int i = 0; i < ramp; i++) {
            static double c = 5;
            long long a = (long long)c;
            c = a / c;
        }
        if (em > 500) {
            em = 0;
            ramp -= 1000;
        }
    }
}

// @endcond