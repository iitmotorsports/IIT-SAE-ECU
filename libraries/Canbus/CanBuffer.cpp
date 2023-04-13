#include "CanBuffer.h"
#include "Canbus.h"
#include "Log.h"

namespace Canbus {

/* Setters */
void Buffer::setDouble(double val) {
    *(double *)buffer = val;
    modified = true;
}
void Buffer::setULong(uint64_t val) {
    *(uint64_t *)buffer = val;
    modified = true;
}
void Buffer::setLong(int64_t val) {
    *(int64_t *)buffer = val;
    modified = true;
}
void Buffer::setFloat(float val, size_t pos) {
    *(float *)(buffer + pos) = val;
    modified = true;
}
void Buffer::setUInt(uint32_t val, size_t pos) {
    *(uint32_t *)(buffer + pos) = val;
    modified = true;
}
void Buffer::setInt(int32_t val, size_t pos) {
    *(int32_t *)(buffer + pos) = val;
    modified = true;
}
void Buffer::setUShort(uint16_t val, size_t pos) {
    *(uint16_t *)(buffer + pos) = val;
    modified = true;
}
void Buffer::setShort(int16_t val, size_t pos) {
    *(int16_t *)(buffer + pos) = val;
    modified = true;
}
void Buffer::setUByte(uint8_t val, size_t pos) {
    buffer[pos] = val;
    modified = true;
}
void Buffer::setByte(int8_t val, size_t pos) {
    *(int8_t *)(buffer + pos) = val;
    modified = true;
}
void Buffer::setBit(bool val, size_t pos) { // pos is in terms of bit position
    buffer[pos / 8] &= val << (pos % 8);
    buffer[pos / 8] |= val << (pos % 8);
    modified = true;
}

/* Getters */
double Buffer::getDouble() {
    return *(double *)buffer;
}
uint64_t Buffer::getULong() {
    return *(uint64_t *)buffer;
}
int64_t Buffer::getLong() {
    return *(int64_t *)buffer;
}
float Buffer::getFloat(size_t pos) {
    return *(float *)(buffer + pos);
}
uint32_t Buffer::getUInt(size_t pos) {
    return *(uint32_t *)(buffer + pos);
}
int32_t Buffer::getInt(size_t pos) {
    return *(int32_t *)(buffer + pos);
}
uint16_t Buffer::getUShort(size_t pos) {
    return *(uint16_t *)(buffer + pos);
}
int16_t Buffer::getShort(size_t pos) {
    return *(int16_t *)(buffer + pos);
}
uint8_t Buffer::getUByte(size_t pos) {
    return buffer[pos];
}
int8_t Buffer::getByte(size_t pos) {
    return *(int8_t *)(buffer + pos);
}
bool Buffer::getBit(size_t pos) { // pos is in terms of bit position
    return (buffer[pos / 8] >> (pos % 8)) & 1;
}
void Buffer::dump(uint8_t *dest) {
    dest[0] = buffer[0];
    dest[1] = buffer[1];
    dest[2] = buffer[2];
    dest[3] = buffer[3];
    dest[4] = buffer[4];
    dest[5] = buffer[5];
    dest[6] = buffer[6];
    dest[7] = buffer[7];
}

void Buffer::set(const uint8_t *src) {
    buffer[0] = src[0];
    buffer[1] = src[1];
    buffer[2] = src[2];
    buffer[3] = src[3];
    buffer[4] = src[4];
    buffer[5] = src[5];
    buffer[6] = src[6];
    buffer[7] = src[7];
}

void Buffer::clear() {
    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;
}

bool Buffer::lock() {
    return !spin_lock.test_and_set(std::memory_order_acquire);
}

int Buffer::lock_wait(unsigned long wait) {
    elapsedMicros em = 0;
    while (spin_lock.test_and_set(std::memory_order_acquire)) { // acquire
        if (wait != 0 && em > wait) {
#ifdef CONF_ECU_DEBUG
            Log.w("Canbus::Buffer", "Lock timed out", address);
#endif
            return -1;
        }

#if defined(__cpp_lib_atomic_flag_test)
        while (spin_lock.test(std::memory_order_relaxed))
            ; // test
#endif
        // spin
    }
    return 0;
}

void Buffer::unlock() {
    spin_lock.clear(std::memory_order_release); // release
}

} // namespace Canbus
