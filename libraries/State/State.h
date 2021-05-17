/**
 * @file State.h
 * @author IR
 * @brief State library
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 * This module is a very basic state machine implementation.
 * 
 * It works by continuously calling State::State_t.run() of each State::State_t child returned.
 * 
 * Starting with the State::State_t passed through State::begin(), when State::State_t.run() returns a valid State::State_t pointer, it will continue to run.
 * 
 * States can notify the next state with an integer by using State::State_t.notify(), where the receiving state calls State::State_t.getNotify(). This number is 0 by default.
 * 
 * When State::State_t.notify() is given NotifyValue::E_FATAL, the state machine will break out of its loop and the state machine will stop.
 * 
 */

#ifndef __ECU_STATE_H__
#define __ECU_STATE_H__

#include <stdint.h>
#include <stdlib.h>
#include <typeinfo>

#include "Log.h"
#include "PPHelp.h"

/**
 * @brief A state machine implementation, refer to State.h for more info.
 * 
 * This library implements a state machine for the Teensy Microcontoller.
 * It was specifically made for the Teensy 3.6.
 */
namespace State {

/**
 * @brief Common codes that can be used by states.
 * @note WIP
 */
enum NotifyValue : int {
    /**
     * @brief No error has occurred.
     */
    E_NOERR,
    /**
     * @brief An error has occurred but we can continue normally.
     */
    E_CONTINUE,
    /**
     * @brief An error has occurred, attempt to skip the state.
     */
    E_ERROR,
    /**
     * @brief Special code that makes the state machine completely stop.
     */
    E_FATAL = 0xFA7A1,
    /**
     * @brief Special code that makes the state machine start from the first state again
     */
    E_RESTART = 2357427,
};

/**
 * @brief The parent state structure to extend from to create more states
 */
struct State_t {
protected:
    /**
     * @brief Get the code of the previous state
     * 
     * @return int notify code
     */
    int getNotify(void);

    /**
     * @brief Send a code to the next state
     * 
     * @param notify code to send
     */
    void notify(int notify);

public:
    /**
     * @brief Used for receiving values from other states
     */
    int notifyCode = 0;

    /**
     * @brief The unique ID of the state, used for logging
     */
    LOG_TAG ID = "ID NOT SET";

    /**
     * @brief Runs the state
     * 
     * @return State_t* A pointer to the next state to switch to
     */
    virtual State_t *run(void);

    /**
     * @brief Returns the LOG_TAG of the state
     * 
     * @return LOG_TAG The unique tag 
     */
    virtual LOG_TAG getID(void);

    /**
     * @brief Get a pointer of the last state
     * @note If the state machine has just started, this will return a nullptr
     * @return State_t* pointer of last state
     */
    State_t *getLastState();
};

/**
 * @brief Begin the state machine with a pointer to a state
 * 
 * @param entry Pointer of the first state to start on
 */
void begin(State_t &entry);

} // namespace State

#endif // __ECU_STATE_H__