/**
 * @file Canbus.cpp
 * @author IR
 * @brief Canbus source file
 * @version 0.2
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
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
static const int TX_MAILBOXES = CONF_FLEXCAN_TX_MAILBOXES;
#undef X

typedef struct CanAddress_t {
    volatile canCallback callback;
    uint32_t address;
    volatile uint8_t buffer[8];
    bool flow;

    CanAddress_t(int address, bool flow) {
        this->address = address;
        this->flow = flow;
    }

} CanAddress_t;

FlexCAN_T4<CONF_FLEXCAN_CAN_SELECT, RX_SIZE_256, TX_SIZE_16> F_Can;

static CanAddress_t addresses[ADDRESS_COUNT + 1] = {
#define X(address, direction) CanAddress_t(address, direction),
    CAN_ADDRESS
#undef X
        CanAddress_t(0xFFFFFFFF, 0), // last entry used as failsafe
};

static CanAddress_t *invalidAddress = addresses + ADDRESS_COUNT;

// NOTE: From what I can tell, a semaphore is needed whenever Can.events is not used, as canMsg handlers will automaticaly run without it.
static volatile uint32_t addressSemaphore = 0xFFFFFFFF; // Address buffer semaphore, // NOTE: address 0xFFFFFFFF cannot be used

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

static CanAddress_t *_getAddress(const uint32_t address) {
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

void copyVolatileCanMsg(volatile uint8_t src[8], uint8_t dest[8]) {
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
static void _receiveCan(const CAN_message_t &msg) { // FIXME: potential issue where checking semaphore freezes, more testing needed
    if (addressSemaphore == msg.id) {               // Throw data away, we are already processing previous msg
#ifdef CONF_ECU_DEBUG
        Log.w(ID, "Discarding can msg", msg.id);
#endif
        return;
    }

    CanAddress_t *addr = _getAddress(msg.id);

    if (addr == invalidAddress) {
        return;
    }

    volatile uint8_t *arr = addr->buffer;
    arr[0] = msg.buf[0];
    arr[1] = msg.buf[1];
    arr[2] = msg.buf[2];
    arr[3] = msg.buf[3];
    arr[4] = msg.buf[4];
    arr[5] = msg.buf[5];
    arr[6] = msg.buf[6];
    arr[7] = msg.buf[7];
    if (addr->callback)
        addr->callback(msg.id, addr->buffer);
}

void addCallback(const uint32_t address, canCallback callback) {
    CanAddress_t *addr = _getAddress(address);
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

void update(void) {
    F_Can.events();
}

void getData(const uint32_t address, uint8_t buf[8]) {
    CanAddress_t *addr = _getAddress(address);
    if (addr != invalidAddress && !addr->flow) { // 0 == incoming
        setSemaphore(address);                   // Semaphore needed to ensure interrupt does not replace data mid transfer
        copyVolatileCanMsg(addr->buffer, buf);
        clearSemaphore();
        return;
    }
    Log.e(ID, "Given address is not incoming or is invalid:", address);
}

volatile uint8_t *getBuffer(const uint32_t address) {
    CanAddress_t *addr = _getAddress(address);
    if (addr == invalidAddress) {
        Log.e(ID, "Address has not been allocated: ", address);
    }
    return addr->buffer;
}

void setSemaphore(const uint32_t address) {
    addressSemaphore = address;
}

void clearSemaphore() { // NOTE: make sure to clear the semaphore BEFORE returning.
    addressSemaphore = 0xFFFFFFFF;
}

static void _pushSendMsg() {
    if (started) // Only do stuff if we have actually enabled canbus
        F_Can.write(send);
}

void pushData(const uint32_t address) {
#ifdef CONF_ECU_DEBUG
    CanAddress_t *addr = _getAddress(address);
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

Canbus::Buffer::Buffer(uint8_t *buffer) {
    this->buffer = buffer;
}

Canbus::Buffer::Buffer(const uint32_t address) {
    this->address = address;
    this->buffer = _getAddress(address)->buffer;
}

void Canbus::Buffer::init() {
    buffer = Canbus::getBuffer(address);
}

void Canbus::Buffer::dump(uint8_t *extBuffer) {
    setSemaphore(address);
    copyVolatileCanMsg(buffer, extBuffer);
    clearSemaphore();
}

void Canbus::Buffer::clear() {
    setSemaphore(address);
    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;
    clearSemaphore();
}

// IMPROVE: remove type-punning, maybe just use bitshifting?

#ifdef CONF_ECU_DEBUG
static void checkInit(Canbus::Buffer *buffer, bool init) {
    if (init) {
        Log.w(Canbus::ID, "Buffer was not initalized before calling a get function", buffer->address);
        buffer->init();
    }
}
#endif

typedef union {
    volatile uint8_t buf[8];
    volatile uint64_t Ulonglong;
    volatile int64_t longlong;
} UBuffer;

uint64_t Canbus::Buffer::getULong() {
#ifdef CONF_ECU_DEBUG
    checkInit(this, buffer == 0);
#endif
    setSemaphore(address);
    UBuffer buf;
    buf.buf[0] = buffer[0];
    buf.buf[1] = buffer[1];
    buf.buf[2] = buffer[2];
    buf.buf[3] = buffer[3];
    buf.buf[4] = buffer[4];
    buf.buf[5] = buffer[5];
    buf.buf[6] = buffer[6];
    buf.buf[7] = buffer[7];
    clearSemaphore();
    return buf.Ulonglong;
}
int64_t Canbus::Buffer::getLong() {
#ifdef CONF_ECU_DEBUG
    checkInit(this, buffer == 0);
#endif
    setSemaphore(address);
    UBuffer buf;
    buf.buf[0] = buffer[0];
    buf.buf[1] = buffer[1];
    buf.buf[2] = buffer[2];
    buf.buf[3] = buffer[3];
    buf.buf[4] = buffer[4];
    buf.buf[5] = buffer[5];
    buf.buf[6] = buffer[6];
    buf.buf[7] = buffer[7];
    clearSemaphore();
    return buf.longlong;
}
uint32_t Canbus::Buffer::getUInt(size_t pos) {
#ifdef CONF_ECU_DEBUG
    checkInit(this, buffer == 0);
#endif
    setSemaphore(address);
    uint32_t val = *(uint32_t *)(buffer + pos);
    clearSemaphore();
    return val;
}
int32_t Canbus::Buffer::getInt(size_t pos) {
#ifdef CONF_ECU_DEBUG
    checkInit(this, buffer == 0);
#endif
    setSemaphore(address);
    int32_t val = *(int32_t *)(buffer + pos);
    clearSemaphore();
    return val;
}
uint16_t Canbus::Buffer::getUShort(size_t pos) {
#ifdef CONF_ECU_DEBUG
    checkInit(this, buffer == 0);
#endif
    setSemaphore(address);
    uint16_t val = *(uint16_t *)(buffer + pos);
    clearSemaphore();
    return val;
}
int16_t Canbus::Buffer::getShort(size_t pos) {
#ifdef CONF_ECU_DEBUG
    checkInit(this, buffer == 0);
#endif
    setSemaphore(address);
    int16_t val = *(int16_t *)(buffer + pos);
    clearSemaphore();
    return val;
}
uint8_t Canbus::Buffer::getUByte(size_t pos) {
#ifdef CONF_ECU_DEBUG
    checkInit(this, buffer == 0);
#endif
    return buffer[pos];
}
int8_t Canbus::Buffer::getByte(size_t pos) {
#ifdef CONF_ECU_DEBUG
    checkInit(this, buffer == 0);
#endif
    return (int8_t)buffer[pos];
}
bool Canbus::Buffer::getBit(size_t pos) {
#ifdef CONF_ECU_DEBUG
    checkInit(this, buffer == 0);
#endif
    return ((bool *)buffer)[pos];
}

// @endcond