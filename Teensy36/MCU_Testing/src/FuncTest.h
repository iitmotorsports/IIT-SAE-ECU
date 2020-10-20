#ifndef __FUNCTEST_H__
#define __FUNCTEST_H__

#include <stdint.h>
#include <stdlib.h>

#include "FlexCAN_T4.h"
#include "config.def"

static CAN_message_t test_msg;

void setTestMessage(void) {
    test_msg.id = random(0, 10);
    test_msg.buf[0] = 'o';
    test_msg.buf[1] = 'h';
    test_msg.buf[2] = ' ';
    test_msg.buf[3] = 'o';
    test_msg.buf[4] = 'k';
    test_msg.buf[5] = 'e';
    test_msg.buf[6] = 'y';
    // test_msg.buf[7] = '';
}

#define sendTestMessage(F_can) \
    setTestMessage();          \
    F_Can.write(test_msg);

void blinkStart() {
    pinMode(13, OUTPUT);
    digitalWriteFast(13, HIGH);
    delay(250);
    digitalWriteFast(13, LOW);
    delay(250);
    digitalWriteFast(13, HIGH);
    delay(250);
    digitalWriteFast(13, LOW);
}

void testSerialBytePrint(void) {

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
    static uint32_t timeout = millis();
    Serial.println("Start Test!");
    while (1) {
        if (millis() - timeout > 100) {

            uint16_t testSpd = random(300);
            test_msg.buf[1] = testSpd & 0xFF;
            test_msg.buf[0] = (testSpd >> 8) & 0xFF;

            Serial.write((const uint8_t *)&test_msg.id, 2);  // Java: DataStream.readInt
            Serial.write((const uint8_t *)&test_msg.buf, 8); // Java: DataStream.readByte x 8
            timeout = millis();
        }
    }
}

void SerialTest(void) {
    blinkStart();
    testSerialBytePrint();
}

void canSniff(const CAN_message_t &msg) {
    Serial.print("MB ");
    Serial.print(msg.mb);
    Serial.print("  OVERRUN: ");
    Serial.print(msg.flags.overrun);
    Serial.print("  LEN: ");
    Serial.print(msg.len);
    Serial.print(" EXT: ");
    Serial.print(msg.flags.extended);
    Serial.print(" TS: ");
    Serial.print(msg.timestamp);
    Serial.print(" ID: ");
    Serial.print(msg.id, HEX);
    Serial.print(" Buffer: ");
    for (uint8_t i = 0; i < msg.len; i++) {
        Serial.print(msg.buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

template <int T>
void readTest(const CAN_message_t &msg) {
    Serial.print(" ID: ");
    Serial.print(msg.id, HEX);
    Serial.print("MB: ");
    Serial.print(T);
    Serial.print(" ");
    for (uint8_t i = 0; i < msg.len; i++) {
        Serial.print((char)msg.buf[i]);
    }
    Serial.println();
}
#endif // __FUNCTEST_H__