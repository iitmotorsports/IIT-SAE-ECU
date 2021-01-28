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
#include "Log.h"

namespace Canbus {

static LOG_TAG ID = "Canbus";

#define X(...) ,
static const int ADDRESS_COUNT = PP_NARG_MO(CAN_ADDRESS);
static const int TX_MAILBOXES = CONF_FLEXCAN_TX_MAILBOXES;
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

static void _setMailboxes() {
    F_Can.setMaxMB(16); // set number of possible TX & RX MBs // NOTE: Teensy 3.6 only has max 16 MBs
    for (int i = 0; i < CONF_FLEXCAN_TX_MAILBOXES; i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, TX, EXT);
    }
    for (int i = CONF_FLEXCAN_TX_MAILBOXES; i < 16; i++) {
        F_Can.setMB((FLEXCAN_MAILBOX)i, RX, EXT);
    }

    int c = 0;
// Auto setup TX & RX MBs
#define X(address, direction)   \
    addressList[c] = address;   \
    addressFlow[c] = direction; \
    c++;
    CAN_ADDRESS
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

static void _receiveCan(const CAN_message_t &msg) {
    if (addressSemaphore == msg.id) // Throw data away, we are already processing previous msg
        return;
    uint pos = _getAddressPos(msg.id);
    memcpy(addressBuffers[pos], msg.buf, 8);
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

void setup(void) {
    _setMailboxes();
    F_Can.setBaudRate(CONF_FLEXCAN_BAUD_RATE);
    F_Can.onReceive(_receiveCan);
    F_Can.enableMBInterrupts();
    F_Can.begin();
    // updateTimer.begin(update, 1); // FIXME: choose an appropriate update time
}

void update(void) { // TODO: Should we update using a timer or just through state loops?
    F_Can.events();
}

void getData(const uint32_t address, uint8_t buf[8]) {
    int pos = _getAddressPos(address);
    if (pos != ADDRESS_COUNT && !addressFlow[pos]) { // 0 == incoming
        setSemaphore(address);                       // Semaphore needed to ensure interrupt does not replace data mid transfer
        memcpy(buf, addressBuffers[pos], 8);         // 8 Bytes // IMPROVE: can we just give the array instead of copying?
        clearSemaphore();
        return;
    }
    Log.e(ID, "Given address is not incoming or is invalid:", address);
}

uint8_t *getBuffer(const uint32_t address) {
    return addressBuffers[_getAddressPos(address)];
}

void setSemaphore(const uint32_t address) {
    addressSemaphore = address;
}

void clearSemaphore() {
    addressSemaphore = 0;
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