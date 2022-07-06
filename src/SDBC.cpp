#include "SDBC.h"
#include "Canbus.h"
#include <bitset>
#include <stdint.h>

#define X(name, id) id,
static uint32_t msg_id[CAN_MESSAGE_NO] = {CAN_IDS};
#undef X
static uint64_t msg[CAN_MESSAGE_NO] = {0};
static bool msg_up[CAN_MESSAGE_NO] = {0};

void update_loop() {
    static size_t clk = 0;
    while (true) {
        clk = (clk + 1) % CAN_MESSAGE_NO;
        if (msg_up[clk]) {
            Canbus::sendData(msg_id[clk], msg[clk]);
            msg_up[clk] = false;
        }
    }
}

void post(int message_index, int bit_size, int bit_pos, uint64_t value) {
    msg[message_index] &= (0 & bit_size) << bit_pos;
    msg[message_index] |= (value & bit_size) << bit_pos;
    msg_up[message_index] = true;
}