/**
 * @file Canbus.h
 * @author IR
 * @brief FlexCAN_T4 wrapper
 * This library is made specifically for SAE
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __ECU_CANBUS_H__
#define __ECU_CANBUS_H__

#include <stdint.h>
#include <stdlib.h>

#include "CanBusAddresses.def"
#include "CanbusConfig.def"
#include "FlexCAN_T4.h"
#include "WProgram.h"

/**
 * @brief Canbus functionality.
 * Refer to Canbus.h for more info.
 */
namespace Canbus {

/**
 * @brief The function type to pass to addCallback
 * 
 * uint32_t The address
 * volatile uint8_t * The 8 byte message buffer
 */
typedef void (*canCallback)(uint32_t, volatile uint8_t *);

/**
 * @brief An incoming canbus message, allows the message data to be interpreted
 */
struct Buffer { // TODO: more rigorous testing on the get funcs
    uint32_t address;
    volatile uint8_t *buffer = 0;
    Buffer(const uint32_t address);
    void init();
    uint32_t getULong();
    int32_t getLong();
    uint16_t getUInt(int pos);
    int16_t getInt(int pos);
    uint8_t getUShort(int pos);
    int8_t getShort(int pos);
};

/**
 * @brief Update the Canbus line
 * This update function will check the rx buffer for any messages and update values.
 * If this function is never run, canbus messages will automaticaly update as they are received
 */
void update(void);

/**
 * @brief Setup the teensy Canbus line
 */
void setup(void);

/**
 * @brief Enable mailbox interrupts, allowing values to automaticaly update.
 * Enabled by default
 * 
 * @param enable boolean
 */
void enableInterrupts(bool enable);

/**
 * @brief Get raw data from a canbus address
 * 
 * @note Only valid incoming addresses will put data onto the given buffer
 * 
 * @param address The incoming address
 * @param buf the buffer to copy data to
 */
void getData(const uint32_t address, uint8_t buf[8]);

/**
 * @brief Get the buffer of an any address.
 * If it is outgoing, use pushData to push the data after modifying the buffer.
 * Invalid addresses will return a buffer that is ignored.
 * @note buffers that are for incoming addresses should not be modified, but can be monitored
 * @note use setSemaphore before using the given pointer while interrupts are active as undefined behavior may occur, alternatively, use a Buffer struct
 * @param address The address
 * @return volatile uint8_t[8] buffer array of the message, length 8
 */
volatile uint8_t *getBuffer(const uint32_t address);

/**
 * @brief Sets whether an address buffer is being actively read from, must be set to the appropriate address if reading directly from a buffer.
 * @note Remember to clearSemaphore once done using the buffer to allow the buffer to be used and updated again.
 * @note Subsequent set semaphores overwrite the previous.
 * 
 * @param address The outgoing address
 */
void setSemaphore(const uint32_t address);

/**
 * @brief Clears any previously set semaphore.
 * 
 */
void clearSemaphore();

/**
 * @brief Add a callback to an incoming address.
 * If an incoming address buffer is updated it will call the given function.
 * Keep callbacks quick and simple.
 * @note Only one callback per address
 * @details If the semaphore is set to the given address the callback will not be called.
 * Semaphores do not have to be used within the function to read from the given buffer.
 * @param address The incoming address
 * @param callback The callback function, refer to canCallback
 */
void addCallback(const uint32_t address, canCallback callback);

/**
 * @brief queue and address's buffer to be pushed.
 * Invalid addresses will not do anything.
 * 
 * @param address The outgoing address
 */
void pushData(const uint32_t address);

/**
 * @brief Send raw data over a given canbus address using a given array
 * 
 * @note Function does not verify that address is outgoing, undefined behavior will occur if data is sent thorugh an incoming address
 * 
 * @param address The outgoing address
 * @param buf The buffer array to be sent
 */
void sendData(const uint32_t address, uint8_t buf[8]);

/**
 * @brief Send raw data over a given canbus address using given values
 * 
 * @note Function does not verify that address is outgoing, undefined behavior will occur if data is sent thorugh an incoming address
 * 
 * @param address The outgoing address
 * @param buf_0 byte 0 of the outgoing buffer
 * @param buf_1 byte 1 of the outgoing buffer
 * @param buf_2 byte 2 of the outgoing buffer
 * @param buf_3 byte 3 of the outgoing buffer
 * @param buf_4 byte 4 of the outgoing buffer
 * @param buf_5 byte 5 of the outgoing buffer
 * @param buf_6 byte 6 of the outgoing buffer
 * @param buf_7 byte 7 of the outgoing buffer
 */
void sendData(const uint32_t address, const uint8_t buf_0 = 0, const uint8_t buf_1 = 0, const uint8_t buf_2 = 0, const uint8_t buf_3 = 0, const uint8_t buf_4 = 0, const uint8_t buf_5 = 0, const uint8_t buf_6 = 0, const uint8_t buf_7 = 0);

/**
 * @brief Copy a volatile canMsg array to a non-volatile array
 * 
 * @param src the volatile source array
 * @param dest the non-volatile destination array
 */
void copyVolatileCanMsg(volatile uint8_t src[8], uint8_t dest[8]);

/**
 * @brief continuously prints out strings of any message that is received through canbus.
 * As such, this function only works when the ECU is in ascii debug mode.
 * 
 */
void enableCanbusSniffer();

} // namespace Canbus

#endif // __ECU_CANBUS_H__