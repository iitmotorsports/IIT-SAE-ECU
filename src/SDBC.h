#include "SDBC.def"

void post(int message_index, int bit_size, int bit_pos, uint64_t value);

template <typename T, typename buf_t>
T sdbc__lookup4(int message_index, int bit_size, int bit_pos) {
    buf_t buf = (msg[message_index] >> bit_pos) & (bit_size - 1);
    return *((T *)&buf);
}

template <typename T, typename buf_t, typename calc_t>
T sdbc__lookup4_s(int message_index, int bit_size, int bit_pos, calc_t scale, calc_t offset) {
    buf_t buf = ((msg[message_index] >> bit_pos) & (bit_size - 1));
    return *((T *)&buf) * scale + offset;
}

#define sdbc__BIT_TYPE(bits) uint##bits##_t

#define sdbc__lookup3(i, sz, pos, post_t, buf_t, ...) sdbc__lookup4<post_t, buf_t>(i, sz, pos)
#define sdbc__lookup3_s(i, sz, pos, post_t, buf_t, scale, offset, calc_t) sdbc__lookup4_s<post_t, buf_t, calc_t>(i, sz, pos, scale, offset)
#define sdbc__lookup2(i, sz, pos, post_t, bits, scale, offset, calc_t, ver) sdbc__lookup3##ver(i, sz, pos, post_t, sdbc__BIT_TYPE(bits), scale, offset, calc_t)
#define sdbc__lookup1(i, sz, pos, meta) sdbc__lookup2(i, sz, pos, meta)
#define lookup(post_id) sdbc__lookup1(post_id, post_id##_META)

#define sdbc__post_type(type, ...) type
#define sdbc_post_type(meta) sdbc__post_type(meta)
#define _pt(post_id) sdbc_post_type(post_id##_META)
#define post_type(post_id) sdbc_post_type(post_id##_META)

void test() {
    _pt(POST_WHEEL_SPEED_FRONT_LEFT) a = lookup(POST_WHEEL_SPEED_FRONT_LEFT);
    a++;
    post(POST_WHEEL_SPEED_FRONT_LEFT, a);
}