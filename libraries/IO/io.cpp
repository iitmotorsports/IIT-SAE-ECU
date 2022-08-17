#include "io.h"

namespace IO {

char *values[8 + SDBC_SOURCE_BITS / 8];
static uint64_t nil = 0xDEADBEEFDEADBEEF;

// TODO: mux each value, given id

#define WRITE_DIGITAL digitalWriteFast
#define WRITE_ANALOG analogWrite

#define READ_DIGITAL digitalReadFast
#define READ_ANALOG analogRead

// FIXME: diff between digital and analog
#define WRITE_GPIO_SELF(name, ID, c_type, index, alt) \
    inline void name(c_type val) {                    \
        *((c_type *)(values + index)) = val;          \
        WRITE_##alt(ID, val);                         \
    }

#define WRITE_VIRT_SELF(name, ID, c_type, index, alt) \
    inline void name(c_type val) {                    \
        *((c_type *)(values + index)) = val;          \
    }

// TODO: Getting static buffer needs to be constexpr
#define WRITE_SIG_SELF(name, ID, c_type, index, alt) \
    inline void name(c_type val) {                   \
        static auto buf = Canbus::getBuffer(ID);     \
        *((c_type *)(buf + index)) = val;            \
        Canbus::pushData(ID);                        \
    }

#define READ_GPIO_SELF(name, ID, c_type, index, alt)           \
    inline c_type name() {                                     \
        return *((c_type *)(values + index)) = READ_##alt(ID); \
    }

#define READ_VIRT_SELF(name, ID, c_type, index, alt) \
    inline c_type name() {                           \
        return *((c_type *)(values + index));        \
    }

// TODO: Getting static buffer needs to be constexpr
#define READ_SIG_SELF(name, ID, c_type, index, alt) \
    inline c_type name() {                          \
        static auto buf = Canbus::getBuffer(ID);    \
        *((c_type *)(buf + index)) = val;           \
        Canbus::pushData(ID);                       \
    }

// TODO: External Source control
#define WRITE_GPIO_EXT(name, ID, c_type, index, alt) \
    inline void name(c_type val) {                   \
    }

#define WRITE_VIRT_EXT(name, ID, c_type, index, alt) \
    inline void name(c_type val) {                   \
    }

#define WRITE_SIG_EXT(name, ID, c_type, index, alt)

#define READ_GPIO_EXT(name, ID, c_type, index, alt) \
    inline void name(c_type val) {                  \
    }

#define READ_VIRT_EXT(name, ID, c_type, index, alt) \
    inline void name(c_type val) {                  \
    }

#define READ_SIG_EXT(name, ID, c_type, index, alt)

#define X(name, ID, c_type, index, e_type, owner) WRITE_##e_type##_##owner(name, ID, c_type, index, alt)

struct __WRITE {

    X(ONBOARD_LED, PINS_ONBOARD_LED, bool, 16, VIRT, SELF)

    inline void ONBOARD_LED(bool val);

    inline void CHARGE_SIGNAL(bool val);

    inline void WHEEL_SPEED_BACK_LEFT(uint32_t data);
};

#undef X
#define X(name, ID, c_type, index, e_type, owner) READ_##e_type##_##owner(name, ID, c_type, index, alt)

struct __READ {
    inline bool ONBOARD_LED();

    inline bool CHARGE_SIGNAL();

    inline uint32_t WHEEL_SPEED_BACK_LEFT();
};

#undef X

// For synced values
/**
 * Sync values
 *
 * Only sync values that are outgoing must reference a VIRT that is set for INPUT or GPIO that is set for OUTPUT
 *
 * Enough addresses must be allocated to fit all the values that are outgoing, other nodes must take note of these addresses where there are relevant values to them
 * As with everything else, these addresses should not overlap with other nodes, or addresses that are in use
 *
 */

struct SyncMsg {
    Canbus::Buffer buf;
    bool update = false;
};

#define __IO_BUF_FUNC_func(func, x) func(x)
#define __IO_BUF_FUNC_double(x, _) __IO_BUF_FUNC_func(setDouble, x)
#define __IO_BUF_FUNC_uint64_t(x, _) __IO_BUF_FUNC_func(setULong, x)
#define __IO_BUF_FUNC_int64_t(x, _) __IO_BUF_FUNC_func(setLong, x)
#define __IO_BUF_FUNC_float setFloat
#define __IO_BUF_FUNC_uint32_t setUInt
#define __IO_BUF_FUNC_int32_t setInt
#define __IO_BUF_FUNC_uint16_t setUShort
#define __IO_BUF_FUNC_int16_t setShort
#define __IO_BUF_FUNC_uint8_t setUByte
#define __IO_BUF_FUNC_int8_t setByte
#define __IO_BUF_FUNC_bool setByte

#define SYNC_SELF_ADDRESSES \
    X(0x46)                 \
    X(0x45)                 \
    X(0x48)

#define SYNC_SELF_ENTRIES \
    X(0x46, bool, CHARGE_SIGNAL, 2)

void send_sync(uint8_t sync_c) {
#define X(addr) static Canbus::Buffer buf##addr(addr);
    SYNC_SELF_ADDRESSES
#undef X
#define X(addr, c_type, name, pos) buf##addr.__IO_BUF_FUNC_##c_type(READ.##name(), pos);
    SYNC_SELF_ENTRIES
#undef X
}

#define X(name, ID, c_type, index, e_type, owner)  \
    case ID:                                       \
        WRITE.##name(*((c_type *)(data + index))); \
        break;

void receive_sync() {
    static auto buf = Canbus::getBuffer(0x42);
}

void run_sync() {
    static uint8_t sync_c = 0;
    while (true) {
        send_sync(sync_c++);
        if (sync_c % 2) {
            static uint c = 0;
            values[sync_i[c]] = ;
            c = (c + 1) % SDBC_SOURCE_NO;
        } else {
            receive_sync();
        }
    }
}

void reset() {
    memset(values, 0, sizeof(values));
    // TODO: set all pins to their corresponding modes.
}
} // namespace IO