#include "Canbus.h"

#include "SDBC.def"
#include "WProgram.h"

namespace IO {

#define _X(node_name)      \
    struct __##node_name##_WRITE { \
    };

EVAL(NODES)

#undef _X

#define X(address, node_name, msg_name, name, bits, pos, stor_t, c_type) inline void CONCAT(##node_name, ##_##name)(c_type val);

// No external can messages can be directly written to, only external synced pins, where they must go under their respective node's name
struct __WRITE {

    EVAL(CAN_SIGNALS)

    inline void ONBOARD_LED(bool val);

    inline void CHARGE_SIGNAL(bool val);

    inline void WHEEL_SPEED_BACK_LEFT(uint32_t data);
};

#undef X
#define X(address, node_name, msg_name, name, bits, pos, stor_t, c_type) inline c_type CONCAT(##node_name, ##_##name)();

// All defined can messages can be read, where external messages are listed under the respective node's name
// External synced pins that can only be read are also listed under the respective node's name
struct __READ {

    EVAL(CAN_SIGNALS)

    inline bool ONBOARD_LED();

    inline bool CHARGE_SIGNAL();

    inline uint32_t WHEEL_SPEED_BACK_LEFT();

    struct __TEST {
        inline bool test();
    } TEST;
}; // namespace IO

static __WRITE WRITE;
static __READ READ;

#undef X

void reset();

} // namespace IO

void test() {
    // READ.FRONT_ECU.
    IO::READ.CHARGE_SIGNAL();
    IO::READ.TEST.test()
}