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
#include "ECUGlobalConfig.h"
#include "Log.h"
#include "PinConfig.def"

// TODO: initialize pins to a set value

namespace Pins {

// NOTE: analogpin indexes extend beyond CORE_NUM_TOTAL_PINS, eg. DAC0
// static int A_GPIO[CORE_NUM_TOTAL_PINS]; // IMPROVE: Use CORE_NUM_ANALOG instead

#define X ,

static const uint analogCanMsgCount_OUT = PP_NARG_MO(PINS_CANBUS_ANALOG_OUT) / 2 + PP_NARG_MO(PINS_CANBUS_ANALOG_OUT) % 2;
static const uint analogCanPinCount_OUT = PP_NARG_MO(PINS_CANBUS_ANALOG_OUT);
static const uint digitalCanPinCount_OUT = PP_NARG_MO(PINS_CANBUS_DIGITAL_OUT);
static const uint analogCanPinCount_IN = PP_NARG_MO(PINS_CANBUS_ANALOG_IN);
static const uint analogCanMsgCount_IN = PP_NARG_MO(PINS_CANBUS_ANALOG_IN) / 2 + PP_NARG_MO(PINS_CANBUS_ANALOG_IN) % 2;
static const uint digitalCanPinCount_IN = PP_NARG_MO(PINS_CANBUS_DIGITAL_IN);

#if PP_NARG_MO(PINS_CANBUS_DIGITAL_OUT) > 8
#error Too many digital out canPins defined
#elif PP_NARG_MO(PINS_CANBUS_DIGITAL_IN) > 8
#error Too many digital in canPins defined
#endif
#undef X

static std::unordered_map<uint8_t, int *> CAN_GPIO_MAP_IN;
static int CAN_GPIO_IN[analogCanPinCount_IN + digitalCanPinCount_IN] = {0}; // Store incoming canpin values
static std::unordered_map<uint8_t, int> CAN_GPIO_MAP_OUT;                   // Store outgoing canpin values
static const int maxActiveDigitalPins = 8;                                  // NOTE: MAX 8 Digital pins per msg for now

static IntervalTimer canbusPinUpdate;
static const LOG_TAG ID = "Pins";

struct CanPinMsg_t {
    uint32_t address;
    uint8_t buf[8] = {0};
    uint64_t *bufmap = (uint64_t *)buf;
};

static int getOutgoingPinValue(uint8_t GPIO_Pin);

// IMPROVE: Make use of all 64 avaliable flags
struct digitalCanPinMsg_t : CanPinMsg_t {
    uint32_t activedigitalCanPins = 0;
    uint8_t digitalPins[maxActiveDigitalPins];
    uint digitalPinPos[maxActiveDigitalPins];
    void send() {
        for (size_t i = 0; i < activedigitalCanPins; i++) {
            *bufmap = *bufmap << 1;
            *bufmap |= (bool)getOutgoingPinValue(digitalPins[i]);
        }
        // Log.d(ID, "Sending Digital Pins", address);
        Canbus::sendData(address, buf);
    };
    void receive(uint8_t buffer[8]) {
        bufmap = (uint64_t *)buffer; // use bufmap, as if we are receiving we can't be sending
        for (int i = activedigitalCanPins - 1; i >= 0; i--) {
            CAN_GPIO_IN[digitalPinPos[i]] = *bufmap & 1;
            Log.d(ID, "Received digital canPin:", digitalPinPos[i]);
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
                *bufmap |= getOutgoingPinValue(analogPins[i]);
        }
        // Log.d(ID, "Sending Analog Pins", address);
        Canbus::sendData(address, buf);
    };
    void receive(uint8_t buffer[8]) {
        bufmap = (uint64_t *)buffer; // use bufmap, as if we are receiving we can't be sending
        int a0 = *bufmap >> 32;
        int a1 = *bufmap & 0x00000000FFFFFFFF;
        CAN_GPIO_IN[analogPinPos[0]] = a0;
        if (analogPins[1] != 255) // Ensure this buffer only allocated one analog val
            CAN_GPIO_IN[analogPinPos[1]] = a1;
        Log.d(ID, "Received analog canPin:", analogPinPos[0]);
        Log.d(ID, "Received analog canPin:", analogPinPos[1]);
    }
};

static digitalCanPinMsg_t digitalCanPinMessage_IN;
static digitalCanPinMsg_t digitalCanPinMessage_OUT;
static analogCanPinMsg_t analogCanPinMessages_IN[analogCanMsgCount_IN];
static analogCanPinMsg_t analogCanPinMessages_OUT[analogCanMsgCount_OUT];

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

// #define __INTERNAL_READ_ANALOG(PIN) A_GPIO[PIN] = analogRead(PIN);
// #define __INTERNAL_READ_DIGITAL(PIN)

static void _receiveDigitalCanbusPin(uint32_t address, uint8_t *buffer) {
    digitalCanPinMessage_IN.receive(buffer);
}

static void _receiveAnalogCanbusPin(uint32_t address, uint8_t *buffer) { // IMPROVE: faster callback for analog pins
    for (size_t i = 0; i < analogCanMsgCount_IN; i++) {
        if (analogCanPinMessages_IN[i].address == address) {
            analogCanPinMessages_IN[i].receive(buffer);
            break;
        }
    }
}

static void _pushCanbusPins(void) {
    digitalCanPinMessage_OUT.send();
    for (size_t i = 0; i < analogCanMsgCount_OUT; i++) {
        analogCanPinMessages_OUT[i].send();
    }
}

void setInternalValue(uint8_t Internal_Pin, int value) {
#ifdef CONF_ECU_DEBUG
    if (CAN_GPIO_MAP_OUT.find(Internal_Pin) != CAN_GPIO_MAP_OUT.end()) {
        Log.d(ID, "Setting Internal pin:", Internal_Pin);
        Log.d(ID, "Setting Internal pin to int:", value);
        CAN_GPIO_MAP_OUT[Internal_Pin] = value;
    }
#else
    if (CAN_GPIO_MAP.find(Internal_Pin) != CAN_GPIO_MAP.end())
        *CAN_GPIO_MAP[Internal_Pin] = value;
#endif
}

static int getOutgoingPinValue(uint8_t GPIO_Pin) {
    if (GPIO_Pin >= 100)
        return CAN_GPIO_MAP_OUT[GPIO_Pin];
    return getPinValue(GPIO_Pin);
}

int getCanPinValue(uint8_t CAN_GPIO_Pin) {
#ifdef CONF_ECU_DEBUG
    if (CAN_GPIO_MAP_IN.find(CAN_GPIO_Pin) != CAN_GPIO_MAP_IN.end()) {
        return *CAN_GPIO_MAP_IN[CAN_GPIO_Pin];
    } else {
        Log.e(ID, "Canpin was not defined before hand:", CAN_GPIO_Pin);
        return 0;
    }
#else
    return *CAN_GPIO_MAP_IN[CAN_GPIO_Pin];
#endif
}

int getPinValue(uint8_t GPIO_Pin) { // IMPROVE: Make getPinValue compatible with canPins
    if (GPIO_Pin >= 100) {          // pins >= 100 are internal pins
        return getCanPinValue(GPIO_Pin);
#define X(pin, Type, IO, init) __READPIN_##Type##IO(pin);
        ECU_PINS
#undef X
    } else {
#ifdef CONF_ECU_DEBUG
        Log.d(ID, "No pin defined", GPIO_Pin);
#endif
        return 0;
    }
}

void setPinValue(uint8_t GPIO_Pin, int value) { // IMPROVE: Make setPinValue compatible with canPins
    if (GPIO_Pin >= 100) {
        return setInternalValue(GPIO_Pin, value);
#define X(pin, Type, IO, init) __WRITEPIN_##Type##IO(pin, value);
        ECU_PINS
#undef X
    }
}

void update(void) {
    // #define X(pin, Type, IO) __INTERNAL_READ_##Type(pin);
    //     ECU_PINS
    // #undef X
}

static void populateCanbusMap(std::multimap<uint32_t, std::tuple<uint, uint8_t, bool>> pinMap, analogCanPinMsg_t *analogCanPinStructArray, uint maxAnalogMsg, digitalCanPinMsg_t *digitalCanPinStruct, uint activeDigitalCanPinCount) {
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
                if (amsgc == maxAnalogMsg) {
                    Log.e(ID, "Exceeded number of allocated analog canbus buffers, maximize the number of analog pins per address", amsgc);
                    break;
                }
                if (ac == 2) {
                    Log.w(ID, "Address has too many analog pins allocated, ignoring additional pins", address);
                    break;
                }
                analogCanPinStructArray[amsgc].address = address;
                analogCanPinStructArray[amsgc].analogPinPos[ac] = std::get<0>(d->second);
                analogCanPinStructArray[amsgc].analogPins[ac] = std::get<1>(d->second);
                ac++;
            } else {              // Digital
                if (dmsgc == 1) { // NOTE: Hardcoded single digital message
                    Log.e(ID, "Exceeded number of allocated digital canbus buffers, currently only one digital buffer is supported", dmsgc);
                    break;
                }
                if (dc == activeDigitalCanPinCount) {
                    Log.w(ID, "Address has too many digital pins allocated, ignoring additional pins", address);
                    break;
                }
                digitalCanPinStruct->address = address;
                digitalCanPinStruct->digitalPinPos[dc] = std::get<0>(d->second);
                digitalCanPinStruct->digitalPins[dc] = std::get<1>(d->second);
                digitalCanPinStruct->activedigitalCanPins = dc; // Set number of active pins out of 8 on digital message
                dc++;
            }
        }

        if (ac || dc) {
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
}

void resetPhysicalPins() {
#define X(pin, Type, IO, init)  \
    pinMode(pin, IO);           \
    if (init != NIL) {          \
        setPinValue(pin, init); \
    }
    ECU_PINS
#undef X
}

void stop(void) {
    canbusPinUpdate.end();
}

void initialize(void) {
    Log.i(ID, "Setting up physical pins");

    analogWriteResolution(12);
    resetPhysicalPins();

    Log.i(ID, "Setting up outgoing canbus pins");
    std::multimap<uint32_t, std::tuple<uint, uint8_t, bool>> pinMap;
    bool isAnalog = true;
    uint i = 0;

#define X(address, pin)                                                        \
    pinMap.insert(std::make_pair(address, std::make_tuple(i, pin, isAnalog))); \
    CAN_GPIO_MAP_OUT[pin] = 0;                                                 \
    i++;
    PINS_CANBUS_ANALOG_OUT
    isAnalog = false;
    PINS_CANBUS_DIGITAL_OUT
#undef X

    Log.i(ID, "Populating outgoing messages");
    populateCanbusMap(pinMap, analogCanPinMessages_OUT, analogCanMsgCount_OUT, &digitalCanPinMessage_OUT, digitalCanPinCount_OUT);

    Log.i(ID, "Setting up incoming canbus pins");
    pinMap.clear();
    isAnalog = true;
    i = 0;

#define X(address, pin)                                                        \
    pinMap.insert(std::make_pair(address, std::make_tuple(i, pin, isAnalog))); \
    CAN_GPIO_MAP_IN[pin] = &CAN_GPIO_IN[i];                                    \
    i++;
    PINS_CANBUS_ANALOG_IN
    isAnalog = false;
    PINS_CANBUS_DIGITAL_IN
#undef X

    Log.i(ID, "Populating incoming messages");
    populateCanbusMap(pinMap, analogCanPinMessages_IN, analogCanMsgCount_IN, &digitalCanPinMessage_IN, digitalCanPinCount_IN);

    Log.i(ID, "Adding incoming canpin callbacks");
    if (digitalCanPinCount_IN != 0)
        Canbus::addCallback(digitalCanPinMessage_IN.address, _receiveDigitalCanbusPin);
    for (size_t i = 0; i < analogCanMsgCount_IN; i++) {
        Canbus::addCallback(analogCanPinMessages_IN[i].address, _receiveAnalogCanbusPin);
    }

    if (digitalCanPinCount_OUT + analogCanPinCount_OUT != 0) {
        Log.i(ID, "Starting outgoing canpin update timer");
        canbusPinUpdate.begin(_pushCanbusPins, CONF_PINS_CANBUS_UPDATE_INTERVAL_MICRO);
    }
#ifdef CONF_ECU_DEBUG
    Serial.print("Printing active canpins");
    for (auto i : CAN_GPIO_MAP_IN) {
        Serial.print(i.first);
        Serial.print(" ");
        Serial.println(*i.second);
    }
#endif
}

} // namespace Pins

// @endcond