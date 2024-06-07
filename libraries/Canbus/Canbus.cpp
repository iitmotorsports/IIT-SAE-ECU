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
#include "SDBC.def"

// IMPROVE: look into filtering only addresses we care about, if it is hardware filtering this should help with bandwidth

namespace CAN {
static const int TX_MAILBOXES = CONFIG_FLEXCAN_TX_MAILBOXES;

FlexCAN_T4<CONFIG_FLEXCAN_CAN_SELECT, RX_SIZE_256, TX_SIZE_16> F_Can;

volatile uint8_t buffer[CAN_MESSAGE_COUNT + 1][8];

#define __BUF_INTERNAL true
#define __BUF_EXTERNAL false
static Buffer buffers[CAN_MESSAGE_COUNT + 1] = {
#define MSG(c, addr, name, sig_c, sig_def, ie_t, contained) Buffer(addr, (volatile uint8_t *)buffer[c], CONCAT(__BUF_, ie_t)),
    CAN_MESSAGES
#undef MSG
        Buffer(0, (volatile uint8_t *)buffer[CAN_MESSAGE_COUNT]),
};

static Buffer *invalidBuf = buffers + CAN_MESSAGE_COUNT;

// Reserved msg objs for sending and receiving
static CAN_message_t receive;
static Thread::Mutex mux_send;
static CAN_message_t send;
static bool started = false;

// FlexCan Callback function
static void _receiveCan(const CAN_message_t &msg) {
    Buffer *buf = Canbus_t::getBuffer(msg.id);

    if (buf == invalidBuf || !buf->lock()) {
#ifdef CONF_ECU_DEBUG
        Log.w(Canbus_t::ID, "Discarding can msg", msg.id);
#endif
        return;
    }

    buf->set(msg.buf);
    if (buf->callback)
        buf->callback(msg.id, buf->buffer);
    buf->unlock();
}

constexpr Buffer *Canbus_t::getBuffer(const uint32_t address) {
    switch (address) {
#define MSG(c, addr, name, sig_c, sig_def, ie_t, contained) \
    case addr:                                           \
        return buffers + c;
        CAN_MESSAGES
#undef MSG
    default:
        return invalidBuf; // Out of range index
    }
}

void Canbus_t::setCallback(const uint32_t address, canCallback callback) {
    Buffer *buf = getBuffer(address);
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

void Canbus_t::sendData(Buffer &buf) {
    mux_send.lock();
    buf.lock_wait();
    send.id = buf.address;
    buf.dump(send.buf);
    buf.unlock();
    F_Can.write(send);
    mux_send.unlock();
}

void Canbus_t::sendData(const uint32_t address, uint8_t buf[8]) {
    mux_send.lock();
    send.id = address;
    memcpy(send.buf, buf, 8); // 8 Bytes
    F_Can.write(send);
    mux_send.unlock();
}

void Canbus_t::sendData(const uint32_t address, const uint8_t buf_0, const uint8_t buf_1, const uint8_t buf_2, const uint8_t buf_3, const uint8_t buf_4, const uint8_t buf_5, const uint8_t buf_6, const uint8_t buf_7) {
    mux_send.lock();
    send.id = address;
    send.buf[0] = buf_0;
    send.buf[1] = buf_1;
    send.buf[2] = buf_2;
    send.buf[3] = buf_3;
    send.buf[4] = buf_4;
    send.buf[5] = buf_5;
    send.buf[6] = buf_6;
    send.buf[7] = buf_7;
    F_Can.write(send);
    mux_send.unlock();
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
    Log.w(ID, "Canbus sniffer will now be outputting raw ascii strings");
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
    F_Can.setBaudRate(CONFIG_FLEXCAN_BAUD_RATE);
    Log.d(ID, "Setting Callback");
    F_Can.onReceive(_receiveCan);
    Log.d(ID, "Enabling Interrupts");
    Thread::sleep(500);         // Just in case canbus needs to do stuff
    F_Can.enableMBInterrupts(); // FIXME: Possible issue where it sometimes freezes here
#ifdef CONF_LOGGING_ASCII_DEBUG
    F_Can.mailboxStatus();
#endif
    started = true;
}

void Canbus_t::run() {
    while (1) {
        for (Buffer &buf : buffers) {
            if (buf.outgoing && buf.modified) { // IMPROVE: only iterate over outgoing
                sendData(buf);
            }
        }
    }
}

} // namespace CAN
  // @endcond