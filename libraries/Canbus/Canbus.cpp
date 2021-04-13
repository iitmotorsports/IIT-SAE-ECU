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

// TODO: look into filtering only addresses we care about

namespace Canbus {
static LOG_TAG ID = "Canbus";

#define X(...) ,
static const int ADDRESS_COUNT = PP_NARG_MO(CAN_ADDRESS);
static const int TX_MAILBOXES = CONF_FLEXCAN_TX_MAILBOXES;
#undef X

FlexCAN_T4<CONF_FLEXCAN_CAN_SELECT, RX_SIZE_256, TX_SIZE_16> F_Can;
// static IntervalTimer updateTimer;
static uint32_t addressList[ADDRESS_COUNT];                   // Sorted list of all the addresses
static volatile uint8_t addressBuffers[ADDRESS_COUNT + 1][8]; // Store buffers for every address, last entry used as failsafe
static bool addressFlow[ADDRESS_COUNT];                       // Denote whether an address is incoming or outgoing, mapped to addressList
static volatile canCallback callbacks[ADDRESS_COUNT] = {0};   // Store any and all callbacks
// NOTE: From what I can tell, a semaphore is needed whenever Can.events is not used, as canMsg handlers will automaticaly run without it.
static volatile uint32_t addressSemaphore = 0; // Address buffer semaphore, // NOTE: address 0x0 cannot be used

// Reserved msg objs for sending and receiving
static CAN_message_t receive;
static CAN_message_t send;

static bool started = false; // used to prevent sending messages when canbus has not been started

static void _swap(uint32_t &a, uint32_t &b) {
    uint32_t temp = a;
    a = b;
    b = temp;
}

static void _swap(bool &a, bool &b) {
    bool temp = a;
    a = b;
    b = temp;
}

static void _selectionSort(uint32_t *array, bool *aux_array, int size) {
    int i, j, imin;
    for (i = 0; i < size - 1; i++) {
        imin = i;
        for (j = i + 1; j < size; j++) {
            if (array[j] < array[imin])
                imin = j;
        }
        _swap(array[i], array[imin]);
        _swap(aux_array[i], aux_array[imin]);
    }
}

static void _setMailboxes() {
    F_Can.setMaxMB(16); // set number of possible TX & RX MBs // NOTE: Teensy 3.6 only has max 16 MBs
    Log.d(ID, "Setting MB RX");
    for (int i = CONF_FLEXCAN_TX_MAILBOXES; i < 16; i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, RX, NONE);
    }
    Log.d(ID, "Setting MB TX");
    for (int i = 0; i < CONF_FLEXCAN_TX_MAILBOXES; i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, TX, NONE);
    }

    Log.d(ID, "Allocating addresses");
    int c = 0;
// Auto setup TX & RX MBs
#define X(address, direction)            \
    addressList[c] = address;            \
    addressFlow[c] = direction;          \
    Log.i(ID, "New address:", address);  \
    Log.i(ID, "Address IO:", direction); \
    c++;
    CAN_ADDRESS
#undef X

    Log.d(ID, "Sorting addresses", ADDRESS_COUNT);
    _selectionSort(addressList, addressFlow, ADDRESS_COUNT);
    Log.d(ID, "Done");
    for (size_t i = 0; i < ADDRESS_COUNT; i++) {
        Log.d(ID, "Sorted address:", addressList[i]);
        Log.d(ID, "Address IO:", addressFlow[i]);
    }
}

// IMPROVE: We have to binary search all mailboxes each time we want to get data
static uint _getAddressPos(const uint32_t address) {
    int s = 0;
    int e = ADDRESS_COUNT;
    while (s <= e) {
        int mid = (s + e) / 2;
        if (addressList[mid] == address) {
            return mid;
        } else if (addressList[mid] > address) {
            e = mid - 1;
        } else {
            s = mid + 1;
        }
    }
    return ADDRESS_COUNT; // Out of range index
}

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
#ifdef CONF_LOGGING_ASCII_DEBUG
        Serial.printf("Discarding can msg %u\n", msg.id);
#endif
        return;
    }

    uint pos = _getAddressPos(msg.id);

    volatile uint8_t *arr = addressBuffers[pos];
    arr[0] = msg.buf[0];
    arr[1] = msg.buf[1];
    arr[2] = msg.buf[2];
    arr[3] = msg.buf[3];
    arr[4] = msg.buf[4];
    arr[5] = msg.buf[5];
    arr[6] = msg.buf[6];
    arr[7] = msg.buf[7];
    if (callbacks[pos])
        callbacks[pos](msg.id, addressBuffers[pos]);
}

void addCallback(const uint32_t address, canCallback callback) {
    uint pos = _getAddressPos(address);
    if (pos == ADDRESS_COUNT) {
        Log.e(ID, "Address has not been allocated: ", address);
        return;
    }

    callbacks[pos] = callback;
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

void setup(void) {
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
    uint pos = _getAddressPos(address);
    if (pos != ADDRESS_COUNT && !addressFlow[pos]) { // 0 == incoming
        setSemaphore(address);                       // Semaphore needed to ensure interrupt does not replace data mid transfer
        copyVolatileCanMsg(addressBuffers[pos], buf);
        clearSemaphore();
        return;
    }
    Log.e(ID, "Given address is not incoming or is invalid:", address);
}

volatile uint8_t *getBuffer(const uint32_t address) {
    uint pos = _getAddressPos(address);
    if (pos == ADDRESS_COUNT) {
        Log.e(ID, "Address has not been allocated: ", address);
    }
    return addressBuffers[pos];
}

void setSemaphore(const uint32_t address) { // FIXME: During testing, teensy froze up when reading a BMS message, fixed when semaphores were disabled
    addressSemaphore = address;
}

void clearSemaphore() {
    addressSemaphore = 0;
}

static void _pushSendMsg() {
    if (started) // Only do stuff if we have actually enabled canbus
        F_Can.write(send);
}

void pushData(const uint32_t address) {
#ifdef CONF_ECU_DEBUG
    uint pos = _getAddressPos(address);
    if (pos == ADDRESS_COUNT) {
        Log.e(ID, "Address has not been allocated: ", address);
        return;
    }
    copyVolatileCanMsg(addressBuffers[pos], send.buf);
#else
    memcpy(send.buf, addressBuffers[_getAddressPos(address)], 8); // 8 Bytes
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

Canbus::Buffer::Buffer(const uint32_t address) {
    this->address = address;
}

void Canbus::Buffer::init() {
    buffer = Canbus::getBuffer(address);
}

// IMPROVE: remove type-punning, maybe just use bitshifting?

uint64_t Canbus::Buffer::getULong() {
    setSemaphore(address);
    uint64_t val = *(uint64_t *)(buffer);
    clearSemaphore();
    return val;
}
int64_t Canbus::Buffer::getLong() {
    setSemaphore(address);
    int64_t val = *(int64_t *)(buffer);
    clearSemaphore();
    return val;
}
uint32_t Canbus::Buffer::getUInt(size_t pos) {
    setSemaphore(address);
    uint32_t val = *(uint32_t *)(buffer + pos);
    clearSemaphore();
    return val;
}
int32_t Canbus::Buffer::getInt(size_t pos) {
    setSemaphore(address);
    int32_t val = *(int32_t *)(buffer + pos);
    clearSemaphore();
    return val;
}
uint16_t Canbus::Buffer::getUShort(size_t pos) {
    setSemaphore(address);
    uint16_t val = *(uint16_t *)(buffer + pos);
    clearSemaphore();
    return val;
}
int16_t Canbus::Buffer::getShort(size_t pos) {
    setSemaphore(address);
    int16_t val = *(int16_t *)(buffer + pos);
    clearSemaphore();
    return val;
}
uint8_t Canbus::Buffer::getUByte(size_t pos) {
    return buffer[pos];
}
int8_t Canbus::Buffer::getByte(size_t pos) {
    return (int8_t)buffer[pos];
}

// @endcond