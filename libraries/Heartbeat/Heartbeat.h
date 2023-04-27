/**
 * @file Heartbeat.h
 * @author IR
 * @brief Make one ECU tell the other it is alive
 * @version 0.1
 * @date 2021-03-19
 *
 * @copyright Copyright (c) 2022
 *
 * @see Heartbeat for more info
 * @see Heartbeat.def for configuration of this module
 */

#ifndef __ECU_HEARTBEAT_H__
#define __ECU_HEARTBEAT_H__

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief A module used to both ensure a connection to both ECUs using CAN
 * and to periodically run callbacks.
 *
 * Currently, Heartbeat::beginBeating() is run on the back ECU and Heartbeat::beginReceiving() is run on the front ECU.
 *
 * Heartbeat::checkBeat() must be polled on the front ECU to log whether or not a heart beat is being detected
 *
 * @see Heartbeat.def for configuration of this module
 */
namespace Heartbeat {

/**
 * @brief Function called each time the *heart* beats
 */
typedef void (*beatFunc)(void);

/**
 * @brief Add a callback to be run at each heartbeat
 *
 * @param func Callback function
 */
void addCallback(beatFunc func);

class CANHeart {
private:
    LOG_TAG ID = "CANHeart";

    beatFunc onBeatFunc = nullptr;

    elapsedMillis lastBeat;

public:
    uint32_t timeCnt = 0;
    const uint32_t timeMult;
    const uint32_t sendingAddress;
    const uint32_t receivingAddress;

    void receiveBeat();

    CANHeart(uint32_t sendingAddress = 0, uint32_t receivingAddress = 0, uint32_t timeMult = 1);
    ~CANHeart();

    /**
     * @brief Poll if a beat has been received
     *
     * @return 1 if the last beat is within allowed delay, 0 otherwise
     */
    int last();

    /**
     * @brief Set a callback to be run at each heartbeat
     *
     * @param func Callback function
     */
    void setCallback(beatFunc func);
};

} // namespace Heartbeat

#endif // __ECU_HEARTBEAT_H__