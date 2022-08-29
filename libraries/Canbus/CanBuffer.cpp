#include "CanBuffer.h"
#include "Canbus.h"

namespace CAN {

/* Setters */
void Buffer::setDouble(double val) {
    *(double *)buffer = val;
}
void Buffer::setULong(uint64_t val) {
    *(uint64_t *)buffer = val;
}
void Buffer::setLong(int64_t val) {
    *(int64_t *)buffer = val;
}
void Buffer::setFloat(float val, size_t pos) {
    *(float *)(buffer + pos) = val;
}
void Buffer::setUInt(uint32_t val, size_t pos) {
    *(uint32_t *)(buffer + pos) = val;
}
void Buffer::setInt(int32_t val, size_t pos) {
    *(int32_t *)(buffer + pos) = val;
}
void Buffer::setUShort(uint16_t val, size_t pos) {
    *(uint16_t *)(buffer + pos) = val;
}
void Buffer::setShort(int16_t val, size_t pos) {
    *(int16_t *)(buffer + pos) = val;
}
void Buffer::setUByte(uint8_t val, size_t pos) {
    buffer[pos] = val;
}
void Buffer::setByte(int8_t val, size_t pos) {
    *(int8_t *)(buffer + pos) = val;
}
void Buffer::setBit(bool val, size_t pos) { // pos is in terms of bit position
    buffer[pos / 8] &= val << (pos % 8);
    buffer[pos / 8] |= val << (pos % 8);
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
void Buffer::dump(uint8_t *extBuffer) {
    Canbus_t::copyVolatileCanMsg(buffer, extBuffer);
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
    return mux.try_lock();
}

void Buffer::lock_wait() {
    mux.lock();
}

void Buffer::unlock() {
    mux.unlock();
}
} // namespace CAN
