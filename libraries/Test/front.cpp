#include "Log.h"
#include "Pins.h"
#include "SerialCommand.h"
#include "WProgram.h"
#include "test.h"

static LOG_TAG ID = "Front Teensy";

void full_front_test() {
    static elapsedMillis timeElapsedHigh;
    static elapsedMillis timeElapsedMidHigh;
    static elapsedMillis timeElapsedMidLow;
    static elapsedMillis timeElapsedLow;

    while (true) {
        if (timeElapsedHigh >= 20) { // High priority updates
            Cmd::receiveCommand();
            timeElapsedHigh = 0;
            static uint32_t speed = 300;
            static bool direction = true;
            if (speed < 50)
                direction = false;
            else if (speed > 250)
                direction = true;
            speed += (direction ? -1 : 1) * random(10);
            Logging::USBHostPush(12, speed); // Current Motor Speed
        }
        if (timeElapsedMidHigh >= 200) { // MedHigh priority updates
            timeElapsedMidHigh = 0;
            Logging::USBHostPush(25, random(5)); // Current State
        }
        if (timeElapsedMidLow >= 500) { // MedLow priority updates
            timeElapsedMidLow = 0;
            static bool on = random(100) > 50;
            Logging::USBHostPush(24, on); // Start Light
        }
        if (timeElapsedLow >= 800) { // Low priority updates
            timeElapsedLow = 0;

            // Motor controllers
            Logging::USBHostPush(4, random(200));  // MC0 DC BUS Voltage
            Logging::USBHostPush(5, random(200));  // MC1 DC BUS Voltage
            Logging::USBHostPush(7, random(200));  // MC0 DC BUS Current
            Logging::USBHostPush(6, random(200));  // MC1 DC BUS Current
            Logging::USBHostPush(9, random(200));  // MC0 Board Temp
            Logging::USBHostPush(8, random(200));  // MC1 Board Temp
            Logging::USBHostPush(11, random(200)); // MC0 Motor Temp
            Logging::USBHostPush(10, random(200)); // MC1 Motor Temp
            Logging::USBHostPush(13, random(200)); // MC Current Power
            // BMS
            Logging::USBHostPush(14, random(200)); // BMS State Of Charge
            Logging::USBHostPush(15, random(200)); // "BMS Immediate Voltage
            Logging::USBHostPush(16, random(200)); // BMS Pack Average Current
            Logging::USBHostPush(17, random(200)); // BMS Pack Highest Temp
            Logging::USBHostPush(18, random(200)); // BMS Pack Lowest Temp
            Logging::USBHostPush(19, random(200)); // BMS Discharge current limit
            Logging::USBHostPush(20, random(200)); // BMS Charge current limit
            // General
            Logging::USBHostPush(21, random(100) > 50); // Fault State
        }
    }
}