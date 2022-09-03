#include "SDBC.h"
#include "Canbus.h"
#include <bitset>
#include <stdint.h>

void post(int message_index, int bit_size, int bit_pos, uint64_t value) {
    msg[message_index] &= (0 & bit_size) << bit_pos;
    msg[message_index] |= (value & bit_size) << bit_pos;
    msg_up[message_index] = true;
}