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

namespace CAN {
#define X(...) ,
static const int ADDRESS_COUNT = PP_NARG_MO(CAN_ADDRESS);
static const int TX_MAILBOXES = CONF_FLEXCAN_TX_MAILBOXES;
#undef X

FlexCAN_T4<CONF_FLEXCAN_CAN_SELECT, RX_SIZE_256, TX_SIZE_16> F_Can;

volatile uint8_t buffers[ADDRESS_COUNT][8];

static Buffer buffer_objs[ADDRESS_COUNT + 1] = {
#define X(address, _) Buffer(address, (volatile uint8_t *)buffers),
    CAN_ADDRESS
#undef X
        Buffer(0),
};

static Buffer *invalidBuf = buffer_objs + ADDRESS_COUNT;

// Reserved msg objs for sending and receiving
static CAN_message_t receive;
static CAN_message_t send;

static bool started = false; // used to prevent sending messages when canbus has not been started

static constexpr Buffer *_getBuffer(const uint32_t address) { // IMPROVE: use a switch, append count to address definitions
    int c = 0;
#define X(addr, direction)      \
    if (addr == address) {      \
        return buffer_objs + c; \
    }                           \
    c++;
    CAN_ADDRESS
#undef X

    return invalidBuf; // Out of range index
} // namespace Canbus

void Canbus_t::copyVolatileCanMsg(volatile uint8_t src[8], uint8_t dest[8]) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
    dest[4] = src[4];
    dest[5] = src[5];
    dest[6] = src[6];
    dest[7] = src[7];
}

// FlexCan Callback function
static void _receiveCan(const CAN_message_t &msg) {
    Buffer *buf = _getBuffer(msg.id);

    if (buf == invalidBuf || !buf->lock()) {
#ifdef CONF_ECU_DEBUG
        Log.w(Canbus_t::ID, "Discarding can msg", msg.id);
#endif
        return;
    }

    volatile uint8_t *arr = buf->buffer;
    arr[0] = msg.buf[0];
    arr[1] = msg.buf[1];
    arr[2] = msg.buf[2];
    arr[3] = msg.buf[3];
    arr[4] = msg.buf[4];
    arr[5] = msg.buf[5];
    arr[6] = msg.buf[6];
    arr[7] = msg.buf[7];
    if (buf->callback)
        buf->callback(msg.id, buf->buffer);
    buf->unlock();
}

void Canbus_t::setCallback(const uint32_t address, canCallback callback) {
    Buffer *buf = _getBuffer(address);
    if (buf == invalidBuf) {
        Log.e(ID, "Address has not been allocated: ", address);
        return;
    }

    buf->lock_wait();
    buf->callback = callback;
    buf->unlock();
}

void Canbus_t::enableInterrupts(bool enable) {
    F_Can.enableMBInterrupts(enable);

#ifdef CONF_ECU_DEBUG
    if (enable) {
        Log.d(ID, "Interrupts enabled");
    } else {
        Log.d(ID, "Interrupts disabled");
    }
#endif
}

void Canbus_t::(const uint32_t address, uint8_t buf[8]) {
    CanAddress_t *addr = _getBuffer(address);
    if (addr != invalidAddress && !addr->flow) { // 0 == incoming
        setSemaphore(address);                   // Semaphore needed to ensure interrupt does not replace data mid transfer
        Canbus_t::copyVolatileCanMsg(addr->buffer, buf);
        clearSemaphore();
        return;
    }
    Log.e(ID, "Given address is not incoming or is invalid:", address);
}

volatile uint8_t *Canbus_t::getBuffer(const uint32_t address) {
    CanAddress_t *addr = _getBuffer(address);
    if (addr == invalidAddress) {
        Log.e(ID, "Address has not been allocated: ", address);
    }
    return addr->buffer;
}

static void _pushSendMsg() {
    if (started) // Only do stuff if we have actually enabled canbus
        F_Can.write(send);
}

void Canbus_t::pushData(const uint32_t address) {
#ifdef CONF_ECU_DEBUG
    CanAddress_t *addr = _getBuffer(address);
    if (addr == invalidAddress) {
        Log.e(ID, "Address has not been allocated: ", address);
        return;
    }
    Canbus_t::copyVolatileCanMsg(addr->buffer, send.buf);
#else
    copyVolatileCanMsg(_getBuffer(address)->buffer, send.buf);
#endif
    send.id = address;
    _pushSendMsg();
}

void Canbus_t::sendData(const uint32_t address, uint8_t buf[8]) {
    send.id = address;
    memcpy(send.buf, buf, 8); // 8 Bytes
    _pushSendMsg();
}

void Canbus_t::sendData(const uint32_t address, const uint8_t buf_0, const uint8_t buf_1, const uint8_t buf_2, const uint8_t buf_3, const uint8_t buf_4, const uint8_t buf_5, const uint8_t buf_6, const uint8_t buf_7) {
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
    Serial.print(" ID: ");
    Serial.print(msg.id, HEX);
    Serial.print(" Buffer: ");
    for (uint8_t i = 0; i < msg.len; i++) {
        Serial.print(msg.buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void Canbus_t::enableCanbusSniffer(bool enable) {
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

void Canbus_t::setup(void) { // IMPROVE: filter only for addresses we care about
    started = false;
    Log.d(ID, "Starting");
    F_Can.begin();      // NOTE: canbus must first be started before it can be configured
    F_Can.setMaxMB(16); // set number of possible TX & RX MBs // NOTE: Teensy 3.6 only has max 16 MBs
    Log.d(ID, "Setting MB RX");
    for (int i = TX_MAILBOXES; i < 16; i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, RX, NONE);
    }
    Log.d(ID, "Setting MB TX");
    for (int i = 0; i < TX_MAILBOXES; i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, TX, NONE);
    }
    F_Can.setBaudRate(CONF_FLEXCAN_BAUD_RATE);
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

void Canbus_t::run() {
    while (1) {
    }
}

} // namespace CAN
  // @endcond