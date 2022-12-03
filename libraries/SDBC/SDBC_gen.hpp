/**
 * AUTO GENERATED FILE - DO NOT EDIT
 *
 * SDBC Values, defined in C++ as their respective ID
 */

#include <stdint.h>

#include "Canbus.h"
#include "ECUGlobalConfig.h"

#include "SDBC_class.hpp"
#include "SDBC_type.hpp"

namespace SDBC {

// All messages, numbers are their addresses
const MSG Messages[6] = {
    {8, false},
    {5, true},
    {2, true},
    {3, true},
    {10, false},
    {0xA0, true},
};

struct FRONT_ECU {
    const int nodeID = 0; // ID based on order of appearance in sdbc
    const struct PIN {
        const GPIO ONBOARD_LED = {13, 1, 1};
        const GPIO ANALOG_TEST = {24, 0, 1};
        const GPIO BUTTON_OFF = {2, 1, 0};
        const GPIO WHEEL_1 = {21, 0, 0};
        const GPIO WHEEL_0 = {20, 0, 0};
        const VIRT CHARGE_SIGNAL = {128 + 0, 1, 1};
        const VIRT START_LED = {128 + 1, 1, 1};
    } PIN;
    const SIG ONBOARD_LED = {0x00000000FFFFFFFF, Messages + 0, &PIN.ONBOARD_LED, dt::int_t, 0, 0};
    const SIG OTHER_CTRL_SIG = {0x0000FFFF00000000, Messages + 0, nullptr, dt::short_t, 32, 0};
    const SIG DEBUG_INTEGER = {0x00000000FFFFFFFF, Messages + 1, nullptr, dt::int_t, 0, 0};
    const SIG WHEEL_SPEED_FRONT_LEFT = {0x00000000FFFFFFFF, Messages + 2, &PIN.WHEEL_0, dt::int_t, 0, 0};
    const SIG WHEEL_SPEED_FRONT_RIGHT = {0xFFFFFFFF00000000, Messages + 2, &PIN.WHEEL_1, dt::int_t, 32, 0};
} FRONT_ECU;

const struct BACK_ECU {
    const int nodeID = 1;
    const struct PIN {
        const GPIO ONBOARD_LED = {13, 1, 1};
        const GPIO SERVO_OFF = {0, 1, 1};
        const GPIO TSV_SIGNAL = {3, 1, 0};
        const GPIO WHEEL_1 = {21, 0, 0};
        const GPIO WHEEL_0 = {20, 0, 0};
        const GPIO AIR2 = {4, 1, 1};
        const VIRT STATE = {128 + 0, 0, 1};
    } PIN;
    const SIG WHEEL_SPEED_BACK_LEFT = {0x00000000FFFFFFFF, Messages + 3, &PIN.WHEEL_0, dt::int_t, 0, 1};
    const SIG WHEEL_SPEED_BACK_RIGHT = {0xFFFFFFFF00000000, Messages + 3, &PIN.WHEEL_1, dt::int_t, 32, 1};
    const SIG ONBOARD_LED = {0x00000000FFFFFFFF, Messages + 4, &PIN.ONBOARD_LED, dt::int_t, 0, 0};
} // namespace SDBC
BACK_ECU;

const struct MC0 {
    const int nodeID = 2;
    const SIG FAULT_GEN_0 = {0xA0, 0};
    const SIG FAULT_GEN_1 = {0xA0, 1};
    const SIG FAULT_GEN_2 = {0xA0, 2};
    const SIG FAULT_GEN_3 = {0xA0, 3};
    const SIG FAULT_GEN_4 = {0xA0, 4};
    const SIG FAULT_GEN_5 = {0xA0, 5};
    const SIG FAULT_GEN_6 = {0xA0, 6};
    const SIG FAULT_GEN_7 = {0xA0, 7};
    const SIG FAULT_GEN_8 = {0xA0, 8};
    const SIG FAULT_GEN_9 = {0xA0, 9};
    const SIG FAULT_GEN_10 = {0xA0, 10};
    const SIG FAULT_GEN_11 = {0xA0, 11};
    const SIG FAULT_GEN_12 = {0xA0, 12};
    const SIG FAULT_GEN_13 = {0xA0, 13};
    const SIG FAULT_GEN_14 = {0xA0, 14};
    const SIG FAULT_GEN_15 = {0xA0, 15};
    const SIG FAULT_GEN_16 = {0xA0, 16};
    const SIG FAULT_GEN_17 = {0xA0, 17};
    const SIG FAULT_GEN_18 = {0xA0, 18};
    const SIG FAULT_GEN_19 = {0xA0, 19};
    const SIG FAULT_GEN_20 = {0xA0, 20};
    const SIG FAULT_GEN_21 = {0xA0, 21};
    const SIG FAULT_GEN_22 = {0xA0, 22};
    const SIG FAULT_GEN_23 = {0xA0, 23};
    const SIG FAULT_GEN_24 = {0xA0, 24};
    const SIG FAULT_GEN_25 = {0xA0, 25};
    const SIG FAULT_GEN_26 = {0xA0, 26};
    const SIG FAULT_GEN_27 = {0xA0, 27};
    const SIG FAULT_GEN_28 = {0xA0, 28};
    const SIG FAULT_GEN_29 = {0xA0, 29};
    const SIG FAULT_GEN_30 = {0xA0, 30};
    const SIG FAULT_GEN_31 = {0xA0, 31};
    const SIG FAULT_GEN_32 = {0xA0, 32};
    const SIG FAULT_GEN_33 = {0xA0, 33};
    const SIG FAULT_GEN_34 = {0xA0, 34};
    const SIG FAULT_GEN_35 = {0xA0, 35};
    const SIG FAULT_GEN_36 = {0xA0, 36};
    const SIG FAULT_GEN_37 = {0xA0, 37};
    const SIG FAULT_GEN_38 = {0xA0, 38};
    const SIG FAULT_GEN_39 = {0xA0, 39};
    const SIG FAULT_GEN_40 = {0xA0, 40};
    const SIG FAULT_GEN_41 = {0xA0, 41};
    const SIG FAULT_GEN_42 = {0xA0, 42};
    const SIG FAULT_GEN_43 = {0xA0, 43};
    const SIG FAULT_GEN_44 = {0xA0, 44};
    const SIG FAULT_GEN_45 = {0xA0, 45};
    const SIG FAULT_GEN_46 = {0xA0, 46};
    const SIG FAULT_GEN_47 = {0xA0, 47};
    const SIG FAULT_GEN_48 = {0xA0, 48};
    const SIG FAULT_GEN_49 = {0xA0, 49};
    const SIG FAULT_GEN_50 = {0xA0, 50};
    const SIG FAULT_GEN_51 = {0xA0, 51};
    const SIG FAULT_GEN_52 = {0xA0, 52};
    const SIG FAULT_GEN_53 = {0xA0, 53};
    const SIG FAULT_GEN_54 = {0xA0, 54};
    const SIG FAULT_GEN_55 = {0xA0, 55};
    const SIG FAULT_GEN_56 = {0xA0, 56};
    const SIG FAULT_GEN_57 = {0xA0, 57};
    const SIG FAULT_GEN_58 = {0xA0, 58};
    const SIG FAULT_GEN_59 = {0xA0, 59};
    const SIG FAULT_GEN_60 = {0xA0, 60};
    const SIG FAULT_GEN_61 = {0xA0, 61};
    const SIG FAULT_GEN_62 = {0xA0, 62};
    const SIG FAULT_GEN_63 = {0xA0, 63};
} MC0;

const struct MC1 {
    const int nodeID = 3;
} MC1;

const struct AMS {
    const int nodeID = 4;
} AMS;

/* ACTIVE NODE SELECTOR */
#ifdef CONFIG_ACTIVE_NODE == FRONT_ECU
#define ACTIVE_NODE SDBC::FRONT_ECU
#endif
// #ifdef CONFIG_ACTIVE_NODE == BACK_ECU
// #define ACTIVE_NODE SDBC::BACK_ECU
// #endif

} // namespace SDBC