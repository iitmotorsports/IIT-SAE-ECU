#ifndef __ECU_CANBUFFER_H__
#define __ECU_CANBUFFER_H__

#include "ECUGlobalConfig.h"
#include <atomic>
#include <stdint.h>
#include <stdlib.h>

namespace Canbus {

/**
 * @brief The function type to pass to addCallback
 *
 * uint32_t The address
 * volatile uint8_t * The 8 byte message buffer
 */
typedef void (*canCallback)(uint32_t, volatile uint8_t *);

/**
 * @brief An incoming canbus message, allows the message data to be interpreted through a simple wrapper class
 * @note Buffers are persistent, when a message is received it stays in the buffer unless cleared
 */
struct Buffer { // IMPROVE: more rigorous testing on the get funcs
    /**
     * @brief Address this buffer represents
     */
    const uint32_t address;

    /**
     * @brief The buffer
     *
     * @warning Not recommended to access directly
     */
    volatile uint8_t buffer[8];

    /**
     * @brief Optional callback function associated with this buffer
     */
    volatile canCallback callback = nullptr;

    /**
     * @brief Whether this buffer is meant to be an outgoing message
     */
    const bool outgoing;

    /**
     * @brief Whether this buffer has been set in anyway
     * @note Must be manually reset
     */
    std::atomic_bool modified = false;

    /**
     * @brief Whether a message has been received from this address
     */
    std::atomic_bool received = false;

    std::atomic_flag spin_lock = false;

    /**
     * @brief Construct a new internal Buffer
     *
     * @param address the address this buffer should represent
     * @param buffer beginning of the array of 8 byte buffers to select from
     */
    Buffer(const uint32_t address, bool outgoing) : address(address), outgoing(outgoing){};

    /**
     * @brief Initialize a buffer if not done so already by constructor
     */
    void init();
    /**
     * @brief Dump the current buffer onto an external one
     *
     * @warning Must externally lock the buffer to ensure atomic operation
     *
     * @param dest the array to dump to
     */
    void dump(uint8_t *dest);
    /**
     * @brief Replace the current buffer
     *
     * @warning Must externally lock the buffer to ensure atomic operation
     *
     * @param src the array to be used
     */
    void set(const uint8_t *src);
    /**
     * @brief Clear the buffer
     *
     * @warning Must externally lock the buffer to ensure atomic operation
     */
    void clear(void);

    void setDouble(double val);
    void setULong(uint64_t val);
    void setLong(int64_t val);
    void setFloat(float val, size_t pos);
    void setUInt(uint32_t val, size_t pos);
    void setInt(int32_t val, size_t pos);
    void setUShort(uint16_t val, size_t pos);
    void setShort(int16_t val, size_t pos);
    void setUByte(uint8_t val, size_t pos);
    void setByte(int8_t val, size_t pos);
    void setBit(bool val, size_t pos);

    double getDouble();

    float getFloat(size_t pos);

    /**
     * @brief Interpret the buffer as an unsigned long
     *
     * @param pos the byte to start interpreting at
     * @return uint64_t The interpreted value
     */
    uint64_t getULong();
    /**
     * @brief Interpret the buffer as a long
     *
     * @param pos the byte to start interpreting at
     * @return int64_t The interpreted value
     */
    int64_t getLong();
    /**
     * @brief Interpret the buffer as an unsigned Integer at byte position `pos`
     *
     * @param pos the byte to start interpreting at
     * @return uint32_t The interpreted value
     */
    uint32_t getUInt(size_t pos);
    /**
     * @brief Interpret the buffer as an Integer at byte position `pos`
     *
     * @param pos the byte to start interpreting at
     * @return int32_t The interpreted value
     */
    int32_t getInt(size_t pos);
    /**
     * @brief Interpret the buffer as an unsigned Short at byte position `pos`
     *
     * @param pos the byte to start interpreting at
     * @return uint16_t The interpreted value
     */
    uint16_t getUShort(size_t pos);
    /**
     * @brief Interpret the buffer as an Short at byte position `pos`
     *
     * @param pos the byte to start interpreting at
     * @return int16_t The interpreted value
     */
    int16_t getShort(size_t pos);
    /**
     * @brief Interpret the buffer as an unsigned Byte at byte position `pos`
     *
     * @param pos the byte to start interpreting at
     * @return uint8_t The interpreted value
     */
    uint8_t getUByte(size_t pos);
    /**
     * @brief Interpret the buffer as a Byte at byte position `pos`
     *
     * @param pos the byte to start interpreting at
     * @return int8_t The interpreted value
     */
    int8_t getByte(size_t pos);
    /**
     * @brief Get the bit at position `pos` of this buffer
     *
     * @param pos the bit to check on the buffer
     * @return bit as boolean
     */
    bool getBit(size_t pos);

    /**
     * @brief Locks this buffer to be used, returns false if it was unable todo so
     *
     * @return true Locked
     * @return false Unable to lock, in use
     */
    bool lock();

    /**
     * @brief locks this buffer to be used, waits indefinitely for it to unlock if it is locked
     *
     * @param wait micros to wait for, 0 for indefinite
     * @return -1 if timeout, else 0
     */
    int lock_wait(unsigned long wait = CONFIG_ECU_BUFFER_TIMEOUT_MICRO);

    /**
     * @brief Unlocks this buffer if it is locked
     */
    void unlock();
};
} // namespace Canbus
#endif // __ECU_CANBUFFER_H__