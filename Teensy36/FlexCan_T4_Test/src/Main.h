#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>
#include <stdlib.h>

#include "FlexCAN_T4.h"
#include "WProgram.h"

#include "Messages.def"
#include "config.def"

#define NUM_TX_MB 3
#define NUM_RX_MB() PP_NARG_MO(CAN_MESSAGES)

FlexCAN_T4<CONF_TEENSY_CAN, RX_SIZE_256, TX_SIZE_16> F_Can;

void setupMB(FLEXCAN_MAILBOX MB, uint32_t address, _MB_ptr handler) {
    F_Can.setMBFilter(MB, address, address);
    F_Can.enhanceFilter(MB);
    F_Can.onReceive(MB, handler);
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