/**
 * @file Echo.cpp
 * @author IR
 * @brief Echo Source File
 * @version 0.1
 * @date 2021-04-14
 * 
 * @copyright Copyright (c) 2021
 * 
 */
//@cond
#include "Echo.h"
#include "Canbus.h"

namespace Echo {

static IntervalTimer currentEcho;
static uint32_t address;
static uint32_t delay;
static uint8_t buf_curr[8];
static int count = 0;

void echo(uint32_t delay, const uint32_t address, uint8_t buf[8]) {
    echo(delay, address, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
}

void echo(uint32_t delay, const uint32_t address, const uint8_t buf_0, const uint8_t buf_1, const uint8_t buf_2, const uint8_t buf_3, const uint8_t buf_4, const uint8_t buf_5, const uint8_t buf_6, const uint8_t buf_7) {
    Canbus::sendData(ADD_ECHO_DATA, buf_0, buf_1, buf_2, buf_3, buf_4, buf_5, buf_6, buf_7);
    uint64_t block = delay;
    block <<= 32;
    block |= address;
    Canbus::sendData(ADD_ECHO_DELAY, (uint8_t *)block);
}

static void send(void) {
    Canbus::sendData(address, buf_curr);
    currentEcho.end();
}

static void startEcho() {
    currentEcho.end();
    currentEcho.begin(send, delay);
}

static void checkStart() {
    count++;
    if (count == 2) {
        startEcho();
        count = 0;
    }
}

static void receiveBlock(uint32_t _, volatile uint8_t *buf) {
    volatile uint64_t block = *((volatile uint64_t *)buf);
    address = block & 0xFFFFFFFF;
    delay = block >> 32;
    checkStart();
}

static void receiveBuffer(uint32_t _, volatile uint8_t buf[8]) {
    buf_curr[0] = buf[0];
    buf_curr[1] = buf[1];
    buf_curr[2] = buf[2];
    buf_curr[3] = buf[3];
    buf_curr[4] = buf[4];
    buf_curr[5] = buf[5];
    buf_curr[6] = buf[6];
    buf_curr[7] = buf[7];
    checkStart();
}

void setup() {
    Canbus::addCallback(ADD_ECHO_DATA, receiveBlock);
    Canbus::addCallback(ADD_ECHO_DELAY, receiveBuffer);
}

} // namespace Echo
  //@endcond