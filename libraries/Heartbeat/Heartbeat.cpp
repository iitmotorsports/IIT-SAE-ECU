/**
 * @file Heartbeat.cpp
 * @author IR
 * @brief Heartbeat source file
 * @version 0.1
 * @date 2021-03-19
 *
 * @copyright Copyright (c) 2022
 *
 */
//@cond

#include "Heartbeat.h"
#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "Heartbeat.def"
#include "Log.h"
#include "Pins.h"
#include "stdint.h"
#include "stdlib.h"
#include <set>
#include <unordered_map>

namespace Heartbeat {

IntervalTimer canbusPinUpdate;
bool started = false;

std::set<beatFunc> auxFuncs;
std::unordered_map<uint32_t, CANHeart *> hearts;

void receiveCAN(uint32_t addr, volatile uint8_t *) {
    if (hearts.find(addr) != hearts.end())
        hearts[addr]->receiveBeat();
}

void beat() {
    Log.d("Heartbeat", "heartbeats", 0, 10000);

    for (auto heart : hearts) {
        if (!(++(heart.second->timeCnt) % heart.second->timeMult))
            Canbus::sendData(heart.second->sendingAddress);
    }

    for (auto f : auxFuncs) {
        f();
    }
}

void CANHeart::setCallback(beatFunc func) {
    onBeatFunc = func;
}

void CANHeart::receiveBeat() {
    lastBeat = 0;
    if (onBeatFunc != nullptr)
        onBeatFunc();
}

CANHeart::CANHeart(uint32_t sendingAddress, uint32_t receivingAddress, uint32_t timeMult) : timeMult(timeMult + !timeMult), sendingAddress(sendingAddress), receivingAddress(receivingAddress) {
    Canbus::addCallback(receivingAddress, receiveCAN);
    hearts[receivingAddress] = this;
    if (!started) {
        started = true;
        canbusPinUpdate.priority(132);
        canbusPinUpdate.begin(beat, CONF_HEARTBEAT_INTERVAL_MILLIS * 1000);
    }
}

CANHeart::~CANHeart() {
    hearts.erase(receivingAddress); // FIXME: Not thread safe
}

int CANHeart::last() {
    if (lastBeat > (CONF_HEARTBEAT_INTERVAL_MILLIS) + CONF_HEARTBEAT_TIMEOUT_MILLI) {
        Log.w(ID, "Heartbeat is taking too long", lastBeat);
        return 0;
    }
    return lastBeat;
}

void addCallback(beatFunc func) {
    auxFuncs.insert(func);
}

} // namespace Heartbeat
  //@endcond