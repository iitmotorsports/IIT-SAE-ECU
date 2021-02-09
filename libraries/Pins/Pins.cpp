/**
 * @file Pins.cpp
 * @author IR
 * @brief Pins source file
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */

// @cond

#include "Pins.h"
#include "IntervalTimer.h"
#include "core_pins.h"
#include "map"
#include "unordered_map"

#include "Canbus.h"
#include "Log.h"
#include "PinConfig.def"

// TODO: initialize pins to a set value

namespace Pins {

static int A_GPIO[CORE_NUM_TOTAL_PINS]; // IMPROVE: Use CORE_NUM_ANALOG instead

#define X ,
static const uint analogCanPinCount = PP_NARG_MO(PINS_CANBUS_ANALOG);
static const uint analogCanMsgCount = PP_NARG_MO(PINS_CANBUS_ANALOG) / 2 + PP_NARG_MO(PINS_CANBUS_ANALOG) % 2;
static const uint digitalCanPinCount = PP_NARG_MO(PINS_CANBUS_DIGITAL);
#undef X

static std::unordered_map<uint8_t, int *> CAN_GPIO_MAP;
static int CAN_GPIO[analogCanPinCount + digitalCanPinCount] = {0};    // Store canpin values
static const uint activedigitalCanPins = min(digitalCanPinCount, 8U); // NOTE: MAX 8 Digital pins per msg

static IntervalTimer canbusPinUpdate;
static const LOG_TAG ID = "Pins";

struct CanPinMsg_t {
    uint32_t address;
    uint8_t buf[8] = {0};
    uint64_t *bufmap = (uint64_t *)buf;
};

// IMPROVE: Make use of all 64 avaliable flags
struct digitalCanPinMsg_t : CanPinMsg_t {
    uint8_t digitalPins[activedigitalCanPins];
    uint digitalPinPos[activedigitalCanPins];
    void send() {
        for (size_t i = 0; i < activedigitalCanPins; i++) {
            *bufmap = *bufmap << 1;
            *bufmap |= (bool)getPinValue(digitalPins[i]);
        }
        Log.d(ID, "Sending Digital Pins", address);
        Canbus::sendData(address, buf);
    };
    void receive(uint8_t buffer[8]) {
        bufmap = (uint64_t *)buffer; // use bufmap, as if we are receiving we can't be sending
        for (int i = activedigitalCanPins - 1; i >= 0; i--) {
            CAN_GPIO[digitalPinPos[i]] = *bufmap & 1;
            *bufmap = *bufmap >> 1;
        }
    }
};

struct analogCanPinMsg_t : CanPinMsg_t {
    uint8_t analogPins[2] = {255, 255}; // NOTE: If pin 255 exists it cannot be used, mostly likely will not happen?
    uint analogPinPos[2];
    void send() {
        *bufmap = 0;
        for (size_t i = 0; i < 2; i++) {
            *bufmap = *bufmap << 32;
            if (analogPins[i] != 255)
                *bufmap |= getPinValue(analogPins[i]);
        }
        Log.d(ID, "Sending Analog Pins", address);
        Canbus::sendData(address, buf);
    };
    void receive(uint8_t buffer[8]) {
        bufmap = (uint64_t *)buffer; // use bufmap, as if we are receiving we can't be sending
        int a0 = *bufmap >> 32;
        int a1 = *bufmap & 0x00000000FFFFFFFF;
        CAN_GPIO[analogPinPos[0]] = a0;
        CAN_GPIO[analogPinPos[1]] = a1;
    }
};

static digitalCanPinMsg_t digitalCanPinMessage;
static analogCanPinMsg_t analogCanPinMessages[analogCanMsgCount];

#define __WRITEPIN_DIGITALOUTPUT(PIN, VAL) \
    }                                      \
    else if (GPIO_Pin == PIN) {            \
        return digitalWriteFast(PIN, VAL);

#define __WRITEPIN_ANALOGOUTPUT(PIN, VAL) \
    }                                     \
    else if (GPIO_Pin == PIN) {           \
        return analogWrite(PIN, VAL);

#define __READPIN_DIGITALINPUT(PIN) \
    }                               \
    else if (GPIO_Pin == PIN) {     \
        return digitalReadFast(PIN);

// IMPROVE: actually implement analog value caching
#define __READPIN_ANALOGINPUT(PIN) \
    }                              \
    else if (GPIO_Pin == PIN) {    \
        return analogRead(PIN);    \
        // return A_GPIO[PIN];

#define __WRITEPIN_DIGITALINPUT(PIN, VAL)
#define __WRITEPIN_ANALOGINPUT(PIN, VAL)
#define __READPIN_DIGITALOUTPUT(PIN)
#define __READPIN_ANALOGOUTPUT(PIN)

#define __INTERNAL_READ_ANALOG(PIN) A_GPIO[PIN] = analogRead(PIN);
#define __INTERNAL_READ_DIGITAL(PIN)

#if PINS_CANBUS_DIRECTION == INPUT

static void _receiveDigitalCanbusPin(uint32_t address, uint8_t *buffer) {
    digitalCanPinMessage.receive(buffer);
}

static void _receiveAnalogCanbusPin(uint32_t address, uint8_t *buffer) { // IMPROVE: faster callback for analog pins
    for (size_t i = 0; i < analogCanMsgCount; i++) {
        if (analogCanPinMessages[i].address == address) {
            analogCanPinMessages[i].receive(buffer);
            break;
        }
    }
}

#else

static void _pushCanbusPins(void) {
    digitalCanPinMessage.send();
    for (size_t i = 0; i < analogCanMsgCount; i++) {
        analogCanPinMessages[i].send();
    }
}

#endif

int getCanPinValue(uint8_t CAN_GPIO_Pin) {
    return *CAN_GPIO_MAP[CAN_GPIO_Pin];
}

int getPinValue(uint8_t GPIO_Pin) {
    if (GPIO_Pin > CORE_NUM_TOTAL_PINS) {
#ifdef CONF_ECU_DEBUG
        Log.e(ID, "Acessing out of range pin", GPIO_Pin);
#endif
        return 0;
#define X(pin, Type, IO) __READPIN_##Type##IO(pin);
        ECU_PINS
#undef X
    } else {
#ifdef CONF_ECU_DEBUG
        Log.d(ID, "No pin defined", GPIO_Pin);
#endif
        return 0;
    }
}

void setPinValue(uint8_t GPIO_Pin, int value) {
    if (GPIO_Pin > CORE_NUM_TOTAL_PINS) {
        return;
#define X(pin, Type, IO) __WRITEPIN_##Type##IO(pin, value);
        ECU_PINS
#undef X
    }
}

void update(void) {
#define X(pin, Type, IO) __INTERNAL_READ_##Type(pin);
    ECU_PINS
#undef X
}

void initialize(void) {
    Log.i(ID, "Setting up physical pins");

#define X(pin, Type, IO) pinMode(pin, IO);
    ECU_PINS
#undef X

    Log.i(ID, "Setting up canbus pins");
    std::multimap<uint32_t, std::tuple<uint, uint8_t, bool>> pinMap;
    bool isAnalog = true;
    uint i = 0;

#define X(address, pin)                                                        \
    pinMap.insert(std::make_pair(address, std::make_tuple(i, pin, isAnalog))); \
    CAN_GPIO_MAP[pin] = &CAN_GPIO[i];                                          \
    i++;
    PINS_CANBUS_ANALOG
    isAnalog = false;
    PINS_CANBUS_DIGITAL
#undef X

    uint amsgc = 0;
    uint dmsgc = 0;
    decltype(pinMap.equal_range(0)) range;
    for (auto i = pinMap.begin(); i != pinMap.end(); i = range.second) {
        range = pinMap.equal_range(i->first);

        uint ac = 0;
        uint dc = 0;
        uint32_t address = i->first;

        for (auto d = range.first; d != range.second; ++d) {
            if (std::get<2>(d->second)) { // Analog
                if (amsgc == analogCanMsgCount) {
                    Log.e(ID, "Exceeded number of allocated analog canbus buffers, maximize the number of analog pins per address", amsgc);
                    break;
                }
                if (ac == 2) {
                    Log.w(ID, "Address has too many analog pins allocated, ignoring additional pins", address);
                    break;
                }
                analogCanPinMessages[amsgc].address = address;
                analogCanPinMessages[amsgc].analogPinPos[ac] = std::get<0>(d->second);
                analogCanPinMessages[amsgc].analogPins[ac] = std::get<1>(d->second);
                ac++;
            } else { // Digital
                if (dmsgc == 1) {
                    Log.e(ID, "Exceeded number of allocated digital canbus buffers, currently only one digital buffer is supported", dmsgc);
                    break;
                }
                if (dc == activedigitalCanPins) {
                    Log.w(ID, "Address has too many digital pins allocated, ignoring additional pins", address);
                    break;
                }
                digitalCanPinMessage.address = address;
                digitalCanPinMessage.digitalPinPos[dc] = std::get<0>(d->second);
                digitalCanPinMessage.digitalPins[dc] = std::get<1>(d->second);
                dc++;
            }
        }

        if (ac | dc) {
            if (ac != 0) {
                amsgc++;
                Log.i(ID, "Set analog canbus buffer:", address);
            }
            if (dc != 0) {
                dmsgc++;
                Log.i(ID, "Set digital canbus buffer:", address);
            }
        } else {
            Log.w(ID, "No pins were set for address: ", address);
        }
    }

#if PINS_CANBUS_DIRECTION == INPUT // Only need to update when pins are outgoing
    Log.i(ID, "Adding Canbus callbacks");
    Canbus::addCallback(digitalCanPinMessage.address, _receiveDigitalCanbusPin);
    for (size_t i = 0; i < analogCanMsgCount; i++) {
        Canbus::addCallback(analogCanPinMessages[i].address, _receiveAnalogCanbusPin);
    }

#else
    Log.i(ID, "Starting canbus pin update timer");
    canbusPinUpdate.begin(_pushCanbusPins, CONF_PINS_CANBUS_UPDATE_INTERVAL_MICRO);
#endif
}

} // namespace Pins

// @endcond