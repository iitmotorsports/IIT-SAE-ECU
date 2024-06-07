/**
 * @file Canbus.h
 * @author IR
 * @brief FlexCAN_T4 wrapper
 * @version 0.1
 * @date 2020-11-11
 *
 * @copyright Copyright (c) 2022
 *
 * This module is a wrapper for the underlying library FlexCAN_T4.
 *
 * This module helps with communication over the [Canbus](https://www.csselectronics.com/screen/page/simple-intro-to-can-bus/language/en) protocol
 *
 */

#ifndef __ECU_CANBUS_H__
#define __ECU_CANBUS_H__

#include <stdint.h>
#include <stdlib.h>

#include "CanBuffer.h"
#include "CanBusAddresses.def"
#include "CanbusConfig.def"
#include "FlexCAN_T4.h"
#include "WProgram.h"
#include "module.hpp"

/**
 * @brief Canbus functionality.
 * Refer to Canbus.h for more info.
 */
namespace CAN {

class Canbus_t : public Module::Module_t {
public:
    inline static LOG_TAG ID = "Canbus";

    using Module::Module_t::Module_t;

    /**
     * @brief Setup the Canbus line
     */
    void setup(void);

    /**
     * @brief Run the teensy Canbus line
     */
    void run(void);

    /**
     * @brief Enable mailbox interrupts, allowing values to automaticaly update.
     * Enabled by default
     *
     * @param enable boolean
     */
    static void enableInterrupts(bool enable);

    /**
     * @brief Get raw data from a canbus address
     *
     * @note Only valid incoming addresses will put data onto the given buffer
     *
     * @param address The incoming address
     * @param buf the buffer to copy data to
     */
    static void getData(const uint32_t address, uint8_t buf[8]);

    /**
     * @brief Get the buffer of an any address.
     * If it is outgoing, use pushData to push the data after modifying the buffer.
     * Invalid addresses will return a buffer that is ignored.
     * @note buffers that are for incoming addresses should not be modified, but can be monitored
     * @note use setSemaphore before using the given pointer while interrupts are active as undefined behavior may occur, alternatively, use a Buffer struct
     * @param address The address
     * @return volatile uint8_t[8] buffer array of the message, length 8
     */
    static constexpr Buffer *getBuffer(const uint32_t address);

    /**
     * @brief Set a callback to an incoming address.
     * If an incoming address buffer is updated it will call the given function.
     * Keep callbacks quick and simple.
     * @note Only one callback per address
     * @details If the semaphore is set to the given address the callback will not be called.
     * Semaphores do not have to be used within the function to read from the given buffer.
     * @param address The incoming address
     * @param callback The callback function, refer to canCallback
     */
    static void setCallback(const uint32_t address, canCallback callback);

    /**
     * @brief queue and address's buffer to be pushed.
     * Invalid addresses will not do anything.
     *
     * @param address The outgoing address
     */
    static void pushData(const uint32_t address);

    /**
     * @brief Send data given a buffer object
     * 
     * @param buf The buffer object to use
     */
    static void sendData(Buffer &buf);

    /**
     * @brief Send raw data over a given canbus address using a given array
     *
     * @note Function does not verify that address is outgoing, undefined behavior will occur if data is sent thorugh an incoming address
     *
     * @param address The outgoing address
     * @param buf The buffer array to be sent
     */
    static void sendData(const uint32_t address, uint8_t buf[8]);

    /**
     * @brief Send raw data over a given canbus address using given values
     *
     * @note Function does not verify that address is outgoing, undefined behavior may occur if data is sent thorugh an incoming address
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
    static void sendData(const uint32_t address, const uint8_t buf_0 = 0, const uint8_t buf_1 = 0, const uint8_t buf_2 = 0, const uint8_t buf_3 = 0, const uint8_t buf_4 = 0, const uint8_t buf_5 = 0, const uint8_t buf_6 = 0, const uint8_t buf_7 = 0);

    /**
     * @brief continuously prints out strings of any message that is received through canbus.
     * As such, this function only works when the ECU is in ascii debug mode.
     */
    static void enableCanbusSniffer(bool enable);
};

} // namespace CAN

const CAN::Canbus_t Canbus;

#endif // __ECU_CANBUS_H__