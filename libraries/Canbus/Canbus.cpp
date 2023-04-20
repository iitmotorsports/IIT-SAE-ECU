/**
 * @file Canbus.cpp
 * @author IR
 * @brief Canbus source file
 * @version 0.2
 * @date 2020-11-11
 *
 * @copyright Copyright (c) 2022
 *
 */

// @cond

#include "Canbus.h"
#include "CanBusAddresses.def"
#include "CanbusConfig.def"
#include "ECUGlobalConfig.h"
#include "Log.h"

// IMPROVE: look into filtering only addresses we care about, if it is hardware filtering this should help with bandwidth

namespace Canbus {
static LOG_TAG ID = "Canbus";

#define X(...) ,
static const int ADDRESS_COUNT = PP_NARG_MO(CAN_ADDRESS);
static const int TX_MAILBOXES = CONFIG_FLEXCAN_TX_MAILBOXES;
#undef X

FlexCAN_T4<CONFIG_FLEXCAN_CAN_SELECT, RX_SIZE_256, TX_SIZE_16> F_Can;

static Buffer addresses[ADDRESS_COUNT + 1] = {
#define X(address, direction) Buffer(address, direction),
    CAN_ADDRESS
#undef X
        Buffer(0xFFFFFFFF, 0), // last entry used as failsafe
};

static Buffer *invalidAddress = addresses + ADDRESS_COUNT;

// Reserved msg objs for sending and receiving
// Reserved msg objs for sending and receiving
static CAN_message_t receive;
static CAN_message_t send;

static bool started = false; // used to prevent sending messages when canbus has not been started

static void _setMailboxes() {
    F_Can.setMaxMB(16); // set number of possible TX & RX MBs // NOTE: Teensy 3.6 only has max 16 MBs
    Log.d(ID, "Setting MB RX");
    for (int i = TX_MAILBOXES; i < 16; i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, RX, NONE);
    }
    Log.d(ID, "Setting MB TX");
    for (int i = 0; i < TX_MAILBOXES; i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, TX, NONE);
    }
}

static Buffer *_getAddress(const uint32_t address) {
    int c = 0;
#define X(addr, direction)    \
    if (addr == address) {    \
        return addresses + c; \
    }                         \
    c++;
    CAN_ADDRESS
#undef X

    return invalidAddress; // Out of range index
} // namespace Canbus

// FlexCan Callback function
static void _receiveCan(const CAN_message_t &msg) { // FIXME: potential issue where checking semaphore freezes, more testing needed
    Buffer *addr = _getAddress(msg.id);
    Buffer::lock l = addr->get_lock();

    if (!l.locked) {
#ifdef CONF_ECU_DEBUG
        Log.w(ID, "Discarding can msg", msg.id);
#endif
        return;
    }

    addr->set(msg.buf);

    if (addr->callback)
        addr->callback(msg.id, addr->buffer);
}

void addCallback(const uint32_t address, canCallback callback) {
    Buffer *addr = _getAddress(address);
    if (addr == invalidAddress) {
        Log.e(ID, "Address has not been allocated: ", address);
        return;
    }

    addr->callback = callback;
}

void enableInterrupts(bool enable) {
    F_Can.enableMBInterrupts(enable);

#ifdef CONF_ECU_DEBUG
    if (enable) {
        Log.d(ID, "Interrupts enabled");
    } else {
        Log.d(ID, "Interrupts disabled");
    }
#endif
}

void setup(void) { // IMPROVE: filter only for addresses we care about
    Log.d(ID, "Starting");
    F_Can.begin(); // NOTE: canbus must first be started before it can be configured
    _setMailboxes();
    F_Can.setBaudRate(CONFIG_FLEXCAN_BAUD_RATE);
    Log.d(ID, "Setting Callback");
    F_Can.onReceive(_receiveCan);
    Log.d(ID, "Enabling Interrupts");
    delay(500);                 // Just in case canbus needs to do stuff
    F_Can.enableMBInterrupts(); // FIXME: Possible issue where it sometimes freezes here
#ifdef CONF_LOGGING_ASCII_DEBUG
    F_Can.mailboxStatus();
#endif
    started = true;
}

void update(void) {
    F_Can.events();
}

int getData(const uint32_t address, uint8_t buf[8]) {
    Buffer *addr = _getAddress(address);
    if (addr != invalidAddress && !addr->outgoing) { // 0 == incoming
        Buffer::lock l = addr->get_lock(Canbus::DEFAULT_TIMEOUT);
        if (!l.locked)
            return -1;
        addr->dump(buf);
        return 0;
    }
    Log.e(ID, "Given address is not incoming or is invalid:", address);
    return -1;
}

Buffer *getBuffer(const uint32_t address) {
    Buffer *addr = _getAddress(address);
    if (addr == invalidAddress) {
        Log.e(ID, "Address has not been allocated: ", address);
        return nullptr;
    }
    return addr;
}

static void _pushSendMsg() {
    if (started) // Only do stuff if we have actually enabled canbus
        F_Can.write(send);
}

void pushData(const uint32_t address) {
#ifdef CONF_ECU_DEBUG
    Buffer *addr = _getAddress(address);
    if (addr == invalidAddress) {
        Log.e(ID, "Address has not been allocated: ", address);
        return;
    }
    copyVolatileCanMsg(addr->buffer, send.buf);
#else
    copyVolatileCanMsg(_getAddress(address)->buffer, send.buf);
#endif
    send.id = address;
    _pushSendMsg();
}

void sendData(const uint32_t address, uint8_t buf[8]) {
    send.id = address;
    memcpy(send.buf, buf, 8); // 8 Bytes
    _pushSendMsg();
}

void sendData(const uint32_t address, const uint8_t buf_0, const uint8_t buf_1, const uint8_t buf_2, const uint8_t buf_3, const uint8_t buf_4, const uint8_t buf_5, const uint8_t buf_6, const uint8_t buf_7) {
    send.id = address;
    send.buf[0] = buf_0;
    send.buf[1] = buf_1;
    send.buf[2] = buf_2;
    send.buf[3] = buf_3;
    send.buf[4] = buf_4;
    send.buf[5] = buf_5;
    send.buf[6] = buf_6;
    send.buf[7] = buf_7;
    _pushSendMsg();
}

static void _canSniff(const CAN_message_t &msg) {
    _receiveCan(msg);
    // Serial.print("MB ");
    // Serial.print(msg.mb);
    // Serial.print("  OVERRUN: ");
    // Serial.print(msg.flags.overrun);
    // Serial.print("  LEN: ");
    // Serial.print(msg.len);
    // Serial.print(" EXT: ");
    // Serial.print(msg.flags.extended);
    // Serial.print(" TS: ");
    // Serial.print(msg.timestamp);
    Serial.print(" ID: ");
    Serial.print(msg.id, HEX);
    Serial.print(" Buffer: ");
    for (uint8_t i = 0; i < msg.len; i++) {
        Serial.print(msg.buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void enableCanbusSniffer(bool enable) {
#ifndef CONF_LOGGING_ASCII_DEBUG
    Log.w(ID, "Canbus sniffer will now be outputing raw ascii strings");
#endif
    if (enable) {
        Serial.println("Enabling canbus sniffer");
        F_Can.onReceive(_canSniff);
    } else {
        Serial.println("Disabling canbus sniffer");
        F_Can.onReceive(_receiveCan);
    }
}

} // namespace Canbus