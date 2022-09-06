#include "io.h"
#include "stdint.h"

namespace IO {

char *pin_buf[PIN_BYTE_ALLOC + 1];

// TODO: mux each value, given id

#define WRITE_DIGITAL digitalWriteFast
#define WRITE_ANALOG analogWrite
#define READ_DIGITAL digitalReadFast
#define READ_ANALOG analogRead
#define WRITE_VIRT_DIGITAL setBit
#define WRITE_VIRT_ANALOG setUInt
#define READ_VIRT_DIGITAL getBit
#define READ_VIRT_ANALOG getUInt

#define TYPE_SEL_ANALOG uint32_t
#define TYPE_SEL_DIGITAL bool

#define __IO_BUF_FUNC_SET_double(x, _) setDouble(x)
#define __IO_BUF_FUNC_SET_uint64_t(x, _) setULong(x)
#define __IO_BUF_FUNC_SET_int64_t(x, _) setLong(x)
#define __IO_BUF_FUNC_SET_float setFloat
#define __IO_BUF_FUNC_SET_uint32_t setUInt
#define __IO_BUF_FUNC_SET_int32_t setInt
#define __IO_BUF_FUNC_SET_uint16_t setUShort
#define __IO_BUF_FUNC_SET_int16_t setShort
#define __IO_BUF_FUNC_SET_uint8_t setUByte
#define __IO_BUF_FUNC_SET_int8_t setByte
#define __IO_BUF_FUNC_SET_bool setBit

#define __IO_BUF_FUNC_GET_double getDouble
#define __IO_BUF_FUNC_GET_uint64_t getULong
#define __IO_BUF_FUNC_GET_int64_t getLong
#define __IO_BUF_FUNC_GET_float getFloat
#define __IO_BUF_FUNC_GET_uint32_t getUInt
#define __IO_BUF_FUNC_GET_int32_t getInt
#define __IO_BUF_FUNC_GET_uint16_t getUShort
#define __IO_BUF_FUNC_GET_int16_t getShort
#define __IO_BUF_FUNC_GET_uint8_t getUByte
#define __IO_BUF_FUNC_GET_int8_t getByte
#define __IO_BUF_FUNC_GET_bool getBit

#define WRITE_GPIO_INTERNAL_ANALOG_INPUT(index, pin_no, name)
#define WRITE_GPIO_INTERNAL_DIGITAL_INPUT(index, pin_no, name)
#define WRITE_GPIO_INTERNAL_ANALOG_OUTPUT(index, pin_no, name) \
    inline void ACTIVE_NODE_WRITE::name(uint32_t val) {        \
        *((uint32_t *)(pin_buf + index)) = val;                \
        WRITE_##ad_t(pin_no, val);                             \
    }
#define WRITE_GPIO_INTERNAL_DIGITAL_OUTPUT(index, pin_no, name) \
    inline void ACTIVE_NODE_WRITE::name(bool val) {             \
        *((bool *)(pin_buf + index)) = val;                     \
        WRITE_##ad_t(pin_no, val);                              \
    }

#define WRITE_VIRT_INTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format) \
    inline void ACTIVE_NODE_WRITE::name(conv_t val) {                                                            \
        static auto buf = Canbus.getBuffer(UID);                                                                 \
        buf->WRITE_VIRT_##ad_t(val, pos);                                                                        \
    }

#define WRITE_SIG_INTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format) \
    inline void ACTIVE_NODE_WRITE::name(conv_t val) {                                                           \
        static auto buf = Canbus.getBuffer(UID);                                                                \
        buf->__IO_BUF_FUNC_SET_##conv_t(val, pos);                                                              \
    }

#define READ_GPIO_INTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format) \
    inline conv_t name() {                                                                                      \
        return *((conv_t *)(values + index)) = READ_##ad_t(ID);                                                 \
    }

#define READ_VIRT_INTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format) \
    inline conv_t name() {                                                                                      \
        static auto buf = Canbus.getBuffer(UID);                                                                \
        return buf->READ_VIRT_##ad_t(pos);                                                                      \
    }

#define READ_SIG_INTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format) \
    inline conv_t name() {                                                                                     \
        static auto buf = Canbus.getBuffer(UID);                                                               \
        return buf->__IO_BUF_FUNC_GET_##conv_t(pos);                                                           \
    }

// TODO: External Source control
#define WRITE_GPIO_EXTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format) \
    inline void name(conv_t val) {                                                                               \
        static auto buf = Canbus.getBuffer(UID);                                                                 \
        buf->WRITE_VIRT_##ad_t(val, pos);                                                                        \
    }

#define WRITE_VIRT_EXTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format) \
    inline void name(conv_t val) {                                                                               \
        static auto buf = Canbus.getBuffer(UID);                                                                 \
        buf->WRITE_VIRT_##ad_t(val, pos);                                                                        \
    }

#define WRITE_SIG_EXTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format)

#define READ_GPIO_EXTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format) \
    inline conv_t name() {                                                                                      \
        static auto buf = Canbus.getBuffer(UID);                                                                \
        return buf->READ_VIRT_##ad_t(pos);                                                                      \
    }

#define READ_VIRT_EXTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format) \
    inline conv_t name() {                                                                                      \
        static auto buf = Canbus.getBuffer(UID);                                                                \
        return buf->READ_VIRT_##ad_t(pos);                                                                      \
    }

#define READ_SIG_EXTERNAL(index, UID, node_name, name, bit_sz, pos, io_t, ie_t, ad_t, conv_t, is_virt, format) \
    inline conv_t name() {                                                                                     \
        static auto buf = Canbus.getBuffer(UID);                                                               \
        return buf->__IO_BUF_FUNC_GET_##conv_t(pos);                                                           \
    }

// WRITE_GPIO_INTERNAL_OUTPUT(0, 0, ONBOARD_LED, DIGITAL, EXPAND_CONCAT(TYPE_SEL_ad_t))

#define PIN(pin_no, node_n, name, io_t, ie_t, ad_t, is_virt) WRITE_GPIO_INTERNAL_##ad_t##_##io_t(56, pin_no, name)
EVAL(ACTIVE_NODE_PINS)
#undef PIN

void receive_sync() {
}

void run_sync() {
}

void reset() {
    memset(pin_buf, 0, sizeof(pin_buf));
    // TODO: set all pins to their corresponding modes.
}
} // namespace IO