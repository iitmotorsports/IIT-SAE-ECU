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
            Log(ID, "Current Motor Speed:", speed);
        }
        if (timeElapsedMidHigh >= 200) { // MedHigh priority updates
            timeElapsedMidHigh = 0;
            Log(ID, "Current State", 0, 500);
        }
        if (timeElapsedMidLow >= 500) { // MedLow priority updates
            timeElapsedMidLow = 0;
            static bool on = random(100) > 50;
            Log(ID, "Start Light", on);
        }
        if (timeElapsedLow >= 800) { // Low priority updates
            timeElapsedLow = 0;

            // Motor controllers
            Log(ID, "MC0 DC BUS Voltage:", random(200));
            Log(ID, "MC1 DC BUS Voltage:", random(200));
            Log(ID, "MC0 DC BUS Current:", random(200));
            Log(ID, "MC1 DC BUS Current:", random(200));
            Log(ID, "MC0 Board Temp:", random(200));
            Log(ID, "MC1 Board Temp:", random(200));
            Log(ID, "MC0 Motor Temp:", random(200));
            Log(ID, "MC1 Motor Temp:", random(200));
            Log(ID, "MC Current Power:", random(200));

            // BMS
            Log(ID, "BMS State Of Charge:", random(100));
            Log(ID, "BMS Immediate Voltage:", random(200));
            Log(ID, "BMS Pack Average Current:", random(200));
            Log(ID, "BMS Pack Highest Temp:", random(200));
            Log(ID, "BMS Pack Lowest Temp:", random(200));
            Log(ID, "BMS Discharge current limit:", random(200));
            Log(ID, "BMS Charge current limit:", random(200));

            // General
            Log(ID, "Fault State", random(100) > 50);
        }
    }
}