/**
 * AUTO GENERATED FILE - DO NOT EDIT
 *
 * SDBC Values, defined in C as their respective ID
 */

#define CAN_MESSAGE_NO 2

#define CAN_WHEEL_SPEED_FRONT_LEFT 0
#define CAN_WHEEL_SPEED_FRONT_RIGHT 1
#define CAN_WHEEL_SPEED_BACK_LEFT 2
#define CAN_WHEEL_SPEED_BACK_RIGHT 3
#define CAN_WHEEL_SPEED_BACK_RIGHT 4, 32, 31

#include <stdint.h>

uint16_t message_ids[CAN_MESSAGE_NO];
uint64_t messages[CAN_MESSAGE_NO];

void post(int id, uint64_t value) {
    uint16_t msg = (id >> 13) & 0x1FFF;
    uint64_t size = (id >> 26) & 0x3F;
    uint16_t shift = (id >> 26) & 0x3F;
    size = (1 << size) - 1;

    messages[msg] &= (0 & size) << shift;
    messages[msg] |= (value & size) << shift;
}

void post(int message_index, int bit_size, int bit_pos, uint64_t value) {
    messages[message_index] &= (0 & bit_size) << bit_pos;
    messages[message_index] |= (value & bit_size) << bit_pos;
}

void main() {
    post(CAN_WHEEL_SPEED_BACK_RIGHT, 4657);
}