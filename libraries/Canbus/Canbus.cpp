/**
 * @file Canbus.cpp
 * @author IR
 * @brief Canbus source file
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */

// @cond

#include "Canbus.h"
#include "CanbusConfig.def"
#include "CanbusMessages.def"

namespace Canbus {

#define X(...) ,
static const int TX_MAILBOXES = CONF_FLEXCAN_TX_MAILBOXES;
static const int RX_MAILBOXES = PP_NARG_MO(CAN_MESSAGES);
#undef X

FlexCAN_T4<CONF_FLEXCAN_CAN_SELECT, RX_SIZE_256, TX_SIZE_16> F_Can;

static void setupMB(FLEXCAN_MAILBOX MB, uint32_t address, _MB_ptr handler) {
    F_Can.setMBFilter(MB, address, address);
    F_Can.enhanceFilter(MB);
    F_Can.onReceive(MB, handler);
}

static void setMailboxes() { // Set mailbox filters & handles from def file

    F_Can.setMaxMB(TX_MAILBOXES + RX_MAILBOXES); // set number of TX & RX MBs

    for (uint8_t i = 0; i < RX_MAILBOXES; i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, RX, NONE);
    }

    for (uint8_t i = RX_MAILBOXES; i < (TX_MAILBOXES + RX_MAILBOXES); i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, TX, NONE);
    }

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

void setup(void) {
    F_Can.begin();
    F_Can.setBaudRate(CONF_FLEXCAN_BAUD_RATE);
    setMailboxes();
}

void update(void) {
    F_Can.events();
}

} // namespace Canbus

// @endcond