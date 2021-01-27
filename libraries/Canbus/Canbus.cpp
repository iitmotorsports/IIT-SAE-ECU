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
#include "CanbusConfig.def"
#include "CanbusMessages.def"
#include "Log.h"

namespace Canbus {

static LOG_TAG ID = "Canbus";

#define X(...) ,
static const int ADDRESS_COUNT = PP_NARG_MO(CAN_MESSAGES);
#undef X

FlexCAN_T4<CONF_FLEXCAN_CAN_SELECT, RX_SIZE_256, TX_SIZE_16> F_Can;
// static IntervalTimer updateTimer;
static uint32_t addressList[ADDRESS_COUNT];          // Sorted list of all the addresses
static uint8_t addressBuffers[ADDRESS_COUNT + 1][8]; // Store buffers for every address, last entry used as failsafe
static bool addressFlow[ADDRESS_COUNT];              // Denote whether an address is incoming or outgoing, mapped to addressList
// NOTE: From what I can tell, a semaphore is needed whenever Can.events is not used, as canMsg handlers will automaticlly run without it.
static uint32_t addressSemaphore = 0; // Address buffer semaphore, // NOTE: address 0x0 cannot be used

// Reserved msg objs for sending and receiving
static CAN_message_t receive;
static CAN_message_t send;

static void _setupMB(const FLEXCAN_MAILBOX MB, const bool outgoing) {
    if (outgoing) {
        Log.d(ID, "Setting up TX MB #", MB);
        F_Can.setMB(MB, TX, NONE);
    } else {
        Log.d(ID, "Setting up RX MB #", MB);
        F_Can.setMB(MB, RX, NONE);
    }
}

// IMPROVE: better auto allocation of mailboxes
static void _setMailboxes() {
    F_Can.setMaxMB(ADDRESS_COUNT); // set number of possible TX & RX MBs

    int MB = 0;
// Auto setup TX & RX MBs
#define X(address, direction)                 \
    _setupMB((FLEXCAN_MAILBOX)MB, direction); \
    addressList[MB] = address;                \
    addressFlow[MB] = direction;              \
    MB++;
    CAN_MESSAGES
#undef X
    for (size_t i = 0; i < ADDRESS_COUNT; i++) { // Selection sort values
        uint32_t add = addressList[i];
        bool add_f = addressFlow[i];
        int min = i;
        for (size_t j = i; j < ADDRESS_COUNT; j++) {
            if (addressList[j] < add) {
                min = j;
            }
        }
        addressList[i] = addressList[min]; // Swap addresses
        addressList[min] = add;
        addressFlow[i] = addressFlow[min]; // Swap direction indicators
        addressFlow[min] = add_f;
    }
}

void setup(void) {
    _setMailboxes();
    F_Can.setBaudRate(CONF_FLEXCAN_BAUD_RATE);
    F_Can.begin();
    F_Can.onReceive(_receiveCan);
    // updateTimer.begin(update, 1); // FIXME: choose an appropriate update time
}

void update(void) { // TODO: Should we update using a timer or just through state loops?
    F_Can.events();
}

static void _receiveCan(const CAN_message_t &msg) {
    if (addressSemaphore == msg.id) // Throw data away, we are already processing previous msg
        return;
    uint pos = _getAddressPos(msg.id);
    memcpy(addressBuffers[pos], msg.buf, 8);
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

void getData(const uint32_t address, uint8_t buf[8]) {
    int pos = _getAddressPos(address);
    if (pos != ADDRESS_COUNT && !addressFlow[pos]) { // 0 == incoming
        addressSemaphore = address;                  // Semaphore needed to ensure interrupt does not replace data mid transfer
        memcpy(buf, addressBuffers[pos], 8);         // 8 Bytes // IMPROVE: can we just give the array instead of copying?
        addressSemaphore = 0;
        return;
    }
    Log.e(ID, "Given address is not incoming or is invalid:", address);
}

uint8_t *getBuffer(const uint32_t address) {
    return addressBuffers[_getAddressPos(address)];
}

static void _pushSendMsg() {
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

void pushData(const uint32_t address) {
    send.id = address;
    memcpy(send.buf, addressBuffers[_getAddressPos(address)], 8); // 8 Bytes
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

} // namespace Canbus

// @endcond