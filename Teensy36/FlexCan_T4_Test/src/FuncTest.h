#ifndef __FUNCTEST_H__
#define __FUNCTEST_H__

#include <stdint.h>
#include <stdlib.h>

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

void sendTestMessage(FlexCAN_T4<CONF_TEENSY_CAN, RX_SIZE_256, TX_SIZE_16> F_Can) {
    setTestMessage();
    F_Can.write(test_msg);
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
    Serial.begin(115200);
    delay(5000);
    static uint32_t timeout = millis();
    Serial.println("Start Test!");
    while (1) {
        if (millis() - timeout > 3000) {
            Serial.write((const uint8_t *)&test_msg.id, 2);  // Java: DataStream.readInt
            Serial.write((const uint8_t *)&test_msg.buf, 8); // Java: DataStream.readByte x 8
            timeout = millis();
        }
    }
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