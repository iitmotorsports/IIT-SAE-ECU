#include "AeroServo.h"
#include "Log.h"
#include "PWMServo.h" // Servo.h is reliant on quick interrupts, switch (and test) if PWMServo is not good enough

namespace Aero {

static const LOG_TAG ID = "AeroServo";

// TODO: check that these values are okay
static const int steerLMin = PINS_VOLT_TO_ANALOG(1.5);
static const int steerLMax = PINS_VOLT_TO_ANALOG(0);
static const int steerRMin = PINS_VOLT_TO_ANALOG(3.5);
static const int steerRMax = PINS_VOLT_TO_ANALOG(5);

static const int angleMin = 30; // 30 is min possible servo angle, 180 is max
static const int angleMax = 72; // angle of attack is 42

static PWMServo servo1;
static PWMServo servo2;

void setup() {
    Log.i(ID, "Initializing Aero servo pins");
    servo1.attach(PINS_BACK_SERVO1_PWM);
    servo2.attach(PINS_BACK_SERVO2_PWM);
    Log.i(ID, "Done");
}

// TODO: Smooth out input values
void run(int breakPressure, int steeringAngle) { // Add 10 millisec delay
    // Brake pressure
    static int breakPos = map(breakPressure, PINS_ANALOG_MIN, PINS_ANALOG_MAX, angleMin, angleMax);

    // Steering angle sensor
    static int steerPos = 0;

    if (steeringAngle <= steerLMin) {
        steerPos = map(steeringAngle, steerLMax, steerLMin, angleMax, angleMin);
    } else if (steeringAngle >= steerRMin) {
        steerPos = map(steeringAngle, steerRMin, steerRMax, angleMin, angleMax);
    }

    // Set position to highest one
    if (breakPos > steerPos) {
        servo2.write(breakPos);
        servo1.write(breakPos);
    } else {
        servo1.write(steerPos);
        servo2.write(steerPos);
    }
}

} // namespace Aero