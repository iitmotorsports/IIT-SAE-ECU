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
#include "Log.h"

namespace Canbus {

static LOG_TAG ID = "Canbus";

#define X(...) ,
static const int TX_MAILBOXES = CONF_FLEXCAN_TX_MAILBOXES;
static const int RX_MAILBOXES = PP_NARG_MO(CAN_MESSAGES);
#undef X

FlexCAN_T4<CONF_FLEXCAN_CAN_SELECT, RX_SIZE_256, TX_SIZE_16> F_Can;

static void setupMB(FLEXCAN_MAILBOX MB, uint32_t address, _MB_ptr handler) {
    F_Can.setMBFilter(MB, address, address);
    F_Can.enhanceFilter(MB);
    F_Can.onReceive(MB, handler);
#if CONF_FLEXCAN_DEBUG
    Log.d(ID, "Setup MB:", address);
#endif
}

// IMPROVE: better auto allocation of mailboxes
static void setMailboxes() { // Set mailbox filters & handles from def file

    F_Can.setMaxMB(TX_MAILBOXES + RX_MAILBOXES); // set number of TX & RX MBs

#if CONF_FLEXCAN_DEBUG
    Log.d(ID, "TX Mailboxes:", TX_MAILBOXES);
    Log.d(ID, "RX Mailboxes:", RX_MAILBOXES);
#endif

    for (uint8_t i = 0; i < RX_MAILBOXES; i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, RX, NONE);
    }

    for (uint8_t i = RX_MAILBOXES; i < (TX_MAILBOXES + RX_MAILBOXES); i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, TX, NONE);
    }

    F_Can.setMBFilter(REJECT_ALL);

    int MB = 0;
// Auto setup requested MBs
#define X(address, func)                         \
    setupMB((FLEXCAN_MAILBOX)MB, address, func); \
    MB++;

    CAN_MESSAGES
#undef X
}

void setup(void) {
    F_Can.begin();
    F_Can.setBaudRate(CONF_FLEXCAN_BAUD_RATE);
    setMailboxes();
}

void update(void) {
    F_Can.events();
}

void enableInterrupts(bool enable) {
    F_Can.enableMBInterrupts(enable);

#if CONF_FLEXCAN_DEBUG
    if (enable) {
        Log.d(ID, "Interrupts enabled");
    } else {
        Log.d(ID, "Interrupts disabled");
    }
#endif
}

static CAN_message_t null_msg; // TODO: Check whether a 0 ID message is a problem
static CAN_message_t receive;  // This is fine?
static CAN_message_t send;     // I think?

CAN_message_t getMailbox(int8_t MB) {
    receive.mb = (FLEXCAN_MAILBOX)MB;
    if (F_Can.readMB(receive)) {
        return receive;
    } else {
        return null_msg;
    }
}

void sendMsg() {
#if CONF_FLEXCAN_DEBUG
    if (F_Can.write(send) == 1) {
        Log.d(ID, "Message Sent", send.id);
    } else {
        Log.d(ID, "Message Queued", send.id);
    }
#else
    F_Can.write(send);
#endif
}

void setHandle(_MB_ptr handler) {
    F_Can.onReceive(handler);
#if CONF_FLEXCAN_DEBUG
    Log.d(ID, "Main handle set");
#endif
}

void clearHandle(void) {
    F_Can.onReceive(nullptr);
#if CONF_FLEXCAN_DEBUG
    Log.d(ID, "Main handle cleared");
#endif
}

} // namespace Canbus

// @endcond