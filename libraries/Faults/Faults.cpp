/**
 * @file Faults.cpp
 * @author IR
 * @brief Faults source file
 * @version 0.1
 * @date 2021-01-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */

// @cond

#include "Faults.h"
#include "Canbus.h"
#include "FaultConfig.def"
#include "Log.h"
#include "Pins.h"

namespace Fault {

static LOG_TAG ID = "Fault Check";

#define X(...) ,
static const int HARD_CAN_COUNT = PP_NARG_MO(HARD_FAULT_ADD);
static const int SOFT_CAN_COUNT = PP_NARG_MO(SOFT_FAULT_ADD);
static const int CAN_COUNT = PP_NARG_MO(HARD_FAULT_ADD) + PP_NARG_MO(SOFT_FAULT_ADD); // Number of Can addresses we need to account for
#undef X

// NOTE: buffers are actually uint8_t buf[8] but to help mask them they are casted to a uint64_t*
static uint64_t *hard_buffers[CAN_COUNT];  // store the incoming message buffers to quickly access
static uint32_t hard_addresses[CAN_COUNT]; // mapped to buffers to later identify what fault occurred
static uint64_t hard_masks[CAN_COUNT];     // mapped to addresses to later identify what fault occurred

static uint64_t *soft_buffers[CAN_COUNT];  // store the incoming message buffers to quickly access
static uint32_t soft_addresses[CAN_COUNT]; // mapped to buffers to later identify what fault occurred
static uint64_t soft_masks[CAN_COUNT];     // mapped to addresses to later identify what fault occurred

// NOTE: these values are overwritten whenever a fault is tripped, whether a hard or soft fault
static uint32_t faulted_address = 0; // Store what address caused a fault if it caused it
static uint64_t faulted_buffer = 0;  // Store the buffer of the message if it caused a fault, must be casted into a uint8_t buf[8]
static uint8_t faulted_pin = 255;

void setup(void) {
    int i = 0;
#define X(add, mask, tag)                                 \
    hard_addresses[i] = add;                              \
    hard_masks[i] = mask;                                 \
    hard_buffers[i] = (uint64_t *)Canbus::getBuffer(add); \
    i++;
    HARD_FAULT_ADD
#undef X
    i = 0;
#define X(add, mask, tag)                                 \
    soft_addresses[i] = add;                              \
    soft_masks[i] = mask;                                 \
    soft_buffers[i] = (uint64_t *)Canbus::getBuffer(add); \
    i++;
    SOFT_FAULT_ADD
#undef X
    Log.d(ID, "Finished setup of buffers");
}

void logFault(void) {
    if (faulted_address) {
        Log.w(ID, "Faulted address:", faulted_address);
        // NOTE: only sending raw buffer for now, split in two as the buffer is a 64bit value and logging only supports 32bit
        Log.w(ID, "Buffer half 0", (uint32_t)faulted_buffer);
        Log.w(ID, "Buffer half 1", (uint32_t)(faulted_buffer << 32));
        // TODO: map and log fault's appropriate identifying strings
        // #define X(add, mask, id)                                   \
//     if (add == faulted_address && mask & faulted_buffer) { \
//         Log.e(ID, id);                                     \
//     }
        //         // Ignore Pre_Build error
        //         ID_FAULT
        // #undef X
    }

#define X(pin, comp, value, id) \
    if (faulted_pin == pin) {   \
        Log.e(ID, id);          \
    }
    HARD_PIN_FAULTS
    SOFT_PIN_FAULTS
#undef X
}

// NOTE: If we want to ensure we check all faults, even after tripping one, we need 2 more arrays
// TODO: Do any canPins need to be checked for faults? Only physical pins can be checked
bool hardFault(void) {
    // Log.d(ID, "Checking for Hard Fault");
    for (size_t i = 0; i < HARD_CAN_COUNT; i++) {
        Canbus::setSemaphore(hard_addresses[i]);
        if (*hard_buffers[i] & hard_masks[i]) {
            faulted_address = hard_addresses[i];
            memcpy(&faulted_buffer, hard_buffers, 8); // 8 byte buffer
            Canbus::clearSemaphore();
#ifdef CONF_ECU_DEBUG
            Log.e(ID, "HardFault address:", faulted_address);
#endif
            return true;
        }
    }
    faulted_address = 0;
    Canbus::clearSemaphore();

#define X(pin, comp, value, id) (((faulted_pin = pin) && Pins::getPinValue(pin)) comp value) ||
    if (HARD_PIN_FAULTS 0) {
#ifdef CONF_ECU_DEBUG
        Log.e(ID, "HardFault pin:", faulted_pin);
#endif
        return true;
    }
#undef X
    faulted_pin = 255;
    // Log.d(ID, "No hard fault tripped");
    return false;
}

bool softFault(void) {
    // Log.d(ID, "Checking for Soft Fault");
    for (size_t i = 0; i < SOFT_CAN_COUNT; i++) {
        Canbus::setSemaphore(soft_addresses[i]);
        if (*soft_buffers[i] & soft_masks[i]) {
            faulted_address = soft_addresses[i];
            memcpy(&faulted_buffer, soft_buffers, 8); // 8 byte buffer
            Canbus::clearSemaphore();
            return true;
        }
    }
    Canbus::clearSemaphore();

#define X(pin, comp, value, id) (((faulted_pin = pin) && Pins::getPinValue(pin)) comp value) ||
    if (SOFT_PIN_FAULTS 0) {
#ifdef CONF_ECU_DEBUG
        Log.e(ID, "SoftFault pin:", faulted_pin);
#endif
        return true;
    }
#undef X
    faulted_pin = 255;

    // Log.d(ID, "No soft fault tripped");
    return false;
}

bool anyFault(void) {
    return hardFault() || softFault();
}

} // namespace Fault
  // @endcond