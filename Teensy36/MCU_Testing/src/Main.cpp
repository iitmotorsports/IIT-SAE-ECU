#include "Main.h"
#include "FuncTest.h"

void setup(void) {
    Serial.begin(CONF_TEENSY_BAUD_RATE);
    delay(CONF_TEENSY_INITAL_DELAY);
    pinMode(6, OUTPUT);
    digitalWrite(6, LOW); /* optional tranceiver enable pin */

    // FlexCan
    F_Can.begin();
    F_Can.setBaudRate(CONF_FLEXCAN_BAUD_RATE);
    setMailboxes();

    Pins::initialize();
}

void loop(void) {
    F_Can.events();
    static uint32_t timeout = millis();
    if (millis() - timeout > 500) {
        sendTestMessage(F_Can);
        Serial.println(Pins::getPinValue(A7));
    }
    if (millis() - timeout > 1000) {
        Pins::setPinValue(A6, random(1024));
    }
    Pins::update();
}

int main(void) {
    // SerialTest();
    setup();
    while (1) {
        loop();
    }
}
