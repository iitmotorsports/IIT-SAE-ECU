#include "Canbus.h"

#include "SDBC.def"
#include "WProgram.h"

namespace IO {

#define ACTIVE_NODE_WRITE EXPAND_CONCAT(ACTIVE_NODE, _WRITE)
#define ACTIVE_NODE_READ EXPAND_CONCAT(ACTIVE_NODE, _READ)
#define ACTIVE_NODE_PINS EXPAND_CONCAT(ACTIVE_NODE, _PINS)
#define ACTIVE_NODE_SYNC_ EXPAND_CONCAT(ACTIVE_NODE, _SYNC_)

#define PIN_ANALOG_INPUT(name)
#define PIN_DIGITAL_INPUT(name)
#define PIN_ANALOG_OUTPUT(name) inline void name(uint32_t val);
#define PIN_DIGITAL_OUTPUT(name) inline void name(bool val);
// GPIO Pin, Node owner name, name of pin, INPUT|OUTPUT, INTERNAL|EXTERNAL, ANALOG|DIGITAL, is virt? true|false
#define PIN(pin_no, node_n, name, io_t, ie_t, ad_t, is_virt) PIN_##ad_t##_##io_t(name)

#define SIG_INTERNAL(address, node_n, name, bits, pos, conv_t, format) inline void name(conv_t val);
#define SIG_EXTERNAL(address, node_n, name, bits, pos, conv_t, format)
// Message address, node_name, name, bit-size, byte/bit pos, INTERNAL|EXTERNAL, conversion type, format
#define __SIG(address, node_n, name, bits, pos, ie_t, conv_t, format) SIG_##ie_t(address, node_n, name, bits, pos, conv_t, format)
#define SIG(address, node_n, name, bits, pos, ie_t, conv_t, format) __SIG(address, node_n, name, bits, pos, ie_t, conv_t, format)

// SYNC NODE struct generator
#define NODE_true_EXTERNAL(node_name)               \
    struct node_name {                              \
        EXPAND_CONCAT(ACTIVE_NODE_SYNC_, node_name) \
    } node_name;
#define NODE_true_INTERNAL(name)
#define NODE_false_INTERNAL(name)
#define NODE_false_EXTERNAL(name)
#define __NODE(node_name, logic, ie_t) NODE_##logic##_##ie_t(node_name)
#define NODE(node_name, logic, ie_t) __NODE(node_name, logic, ie_t)

// MSG signal extractor/container
#define MSG_true(c, addr, name, sig_c, sig_def, ie_t) \
    struct name {                                     \
        sig_def                                       \
    } name;
#define MSG_false(c, addr, name, sig_c, sig_def, ie_t) sig_def

#define MSG(c, addr, name, sig_c, sig_def, ie_t, contained) MSG_##contained(c, addr, name, sig_c, sig_def, ie_t)

// NODE WRITE
struct ACTIVE_NODE_WRITE {
    ACTIVE_NODE_PINS
    EXPAND_CONCAT(ACTIVE_NODE, _MSGS)
    NODES
};

#undef PIN_ANALOG_INPUT
#undef PIN_DIGITAL_INPUT
#undef PIN_ANALOG_OUTPUT
#undef PIN_DIGITAL_OUTPUT
#undef SIG_INTERNAL
#undef SIG_EXTERNAL
#undef NODE_true_EXTERNAL
#undef NODE_false_EXTERNAL

#define PIN_ANALOG_INPUT(name) inline uint32_t name();
#define PIN_DIGITAL_INPUT(name) inline bool name();
#define PIN_ANALOG_OUTPUT(name) inline uint32_t name();
#define PIN_DIGITAL_OUTPUT(name) inline bool name();
#define SIG_INTERNAL(address, node_n, name, bits, pos, conv_t, format) inline conv_t name();
#define SIG_EXTERNAL(address, node_n, name, bits, pos, conv_t, format) inline conv_t name();

// Expose CAN signals
#define NODE_true_EXTERNAL(node_name)               \
    struct node_name {                              \
        EXPAND_CONCAT(ACTIVE_NODE_SYNC_, node_name) \
        EXPAND_CONCAT(node_name, _MSGS)             \
    } node_name;

// Expose CAN signals
#define NODE_false_EXTERNAL(node_name)  \
    struct node_name {                  \
        EXPAND_CONCAT(node_name, _MSGS) \
    } node_name;

// NODE READ
struct ACTIVE_NODE_READ {
    ACTIVE_NODE_PINS
    EXPAND_CONCAT(ACTIVE_NODE, _MSGS)
    EVAL(NODES)
};

static ACTIVE_NODE_WRITE WRITE;
static ACTIVE_NODE_READ READ;

void reset();

#undef __NODE
#undef NODE
#undef MSG
#undef MSG_true
#undef MSG_false
#undef __SIG
#undef SIG_INTERNAL
#undef SIG_EXTERNAL
#undef PIN
#undef PIN_ANALOG_INPUT
#undef PIN_DIGITAL_INPUT
#undef PIN_ANALOG_OUTPUT
#undef PIN_DIGITAL_OUTPUT

} // namespace IO

void test() {
    IO::WRITE.WHEEL_SPEED_BACK_LEFT(25);
    IO::WRITE.ONBOARD_LED(true);
    IO::WRITE.FRONT_ECU.ONBOARD_LED(true);
    IO::READ.FRONT_ECU.WHEEL_SPEED_FRONT_LEFT();
    IO::READ.MC0.FAULTS.FAULT_GEN_5();
}