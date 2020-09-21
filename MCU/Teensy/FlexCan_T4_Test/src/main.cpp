#include "main.h"

void setup(void) {
    Serial.begin(115200);
    delay(1000);
    pinMode(13, OUTPUT);
    pinMode(6, OUTPUT);
    digitalWrite(6, LOW); /* optional tranceiver enable pin */

    // FlexCan
    F_Can.begin();
    F_Can.setBaudRate(1000000);
    //Can1.enableFIFO();
    //Can1.enableFIFOInterrupt();
    // Can1.onReceive(FIFO, canSniff);

    setMailboxes();
}

void loop() {
    F_Can.events();
    static uint32_t timeout = millis();
    if (millis() - timeout > 1000) {
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
