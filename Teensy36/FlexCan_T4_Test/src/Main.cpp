#include "Main.h"
#include "TimerThree.h"

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

void test() {
    struct test_msg {
        uint32_t id = 0x0102;
        uint8_t buf[8] = {
            0xDE,
            0xAD, // Convert to Short = ‭57005‬
            0,
            0xDE, // Convert to long = ‭3735928559‬
            0xAD,
            0xBE,
            0xEF,
            0,
        };
    } test_msg;
    Serial.begin(115200);
    delay(5000);
    static uint32_t timeout = millis();
    Serial.println("Start!");
    while (1) {
        if (millis() - timeout > 3000) {
            Serial.write((const uint8_t *)&test_msg.id, 2);  // Java: DataStream.readInt
            Serial.write((const uint8_t *)&test_msg.buf, 8); // Java: DataStream.readByte x 8
            timeout = millis();
        }
    }
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
    // setup();
    // while (1) {
    //     loop();
    // }
    test();
}
