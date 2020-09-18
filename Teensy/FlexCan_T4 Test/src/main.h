#define USB_SERIAL // Get rid of IDE errors, they are annoying
#define __MK66FX1M0__

#include "FlexCAN_T4.h"
#include "WProgram.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef __MAIN_H__
#define __MAIN_H__

#define NUM_TX_MAILBOXES 3
#define NUM_RX_MAILBOXES 3

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
static CAN_message_t test_msg;

void setTestMessage() {
    test_msg.id = random(0, 11);
    test_msg.buf[0] = 'o';
    test_msg.buf[1] = 'h';
    test_msg.buf[2] = ' ';
    test_msg.buf[3] = 'o';
    test_msg.buf[4] = 'k';
    test_msg.buf[5] = 'e';
    test_msg.buf[6] = 'y';
    // test_msg.buf[7] = '';
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
void readCan(const CAN_message_t &msg) {
    Serial.print("MB: ");
    Serial.print(T);
    Serial.print(" ");
    Serial.print("ID: ");
    Serial.print(msg.id);
    Serial.print(" ");
    for (uint8_t i = 0; i < msg.len; i++) {
        Serial.print((char)msg.buf[i]);
    }
    Serial.println();
}

#endif // __MAIN_H__