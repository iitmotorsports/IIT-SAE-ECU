#include "FlexCAN_T4.h"
#include "WProgram.h"
#include "messages.def"
#include <stdint.h>
#include <stdlib.h>

#define NUM_TX_MB 3
#define NUM_RX_MB() PP_NARG_MO(CAN_MESSAGES)
#define TEENSY_CAN CAN1

#ifndef __MAIN_H__
#define __MAIN_H__

FlexCAN_T4<TEENSY_CAN, RX_SIZE_256, TX_SIZE_16> F_Can;
static CAN_message_t test_msg;

void setupMB(FLEXCAN_MAILBOX MB, uint32_t address, _MB_ptr handler) {
    F_Can.setMBFilter(MB, address, address);
    F_Can.enhanceFilter(MB);
    F_Can.onReceive(MB, handler);
}

void setTestMessage() {
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

void sendTestMessage() {
    setTestMessage();
    F_Can.write(test_msg);
}

void setMailboxes() { // Set mailbox filters & handles from def file
// Auto set number of TX &RX MBs
#define X(...) ,
    F_Can.setMaxMB(NUM_TX_MB + NUM_RX_MB());
    for (uint8_t i = 0; i < NUM_RX_MB(); i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, RX, NONE);
    }
    for (uint8_t i = NUM_RX_MB(); i < (NUM_TX_MB + NUM_RX_MB()); i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, TX, NONE);
    }
#undef X

    F_Can.setMBFilter(REJECT_ALL);
    F_Can.enableMBInterrupts();

    int MB = 0;
// Auto setup requested MBs
#define X(address, func)                         \
    setupMB((FLEXCAN_MAILBOX)MB, address, func); \
    MB++;

    CAN_MESSAGES
#undef X

    F_Can.mailboxStatus();
}

#endif // __MAIN_H__