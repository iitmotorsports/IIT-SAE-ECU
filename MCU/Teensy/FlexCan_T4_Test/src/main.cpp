#include "Main.h"

void setup(void) {
    Serial.begin(115200);
    delay(1000);
    pinMode(6, OUTPUT);
    digitalWrite(6, LOW); /* optional tranceiver enable pin */

    // FlexCan
    F_Can.begin();
    F_Can.setBaudRate(1000000);
    setMailboxes();
}

void loop() {
    F_Can.events();
    static uint32_t timeout = millis();
    if (millis() - timeout > 500) {
        sendTestMessage();
        timeout = millis();
    }
}

int main(void) {
    setup();
    while (1) {
        loop();
    }
}
