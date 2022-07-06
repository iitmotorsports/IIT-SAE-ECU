#include "SDBC.def"

void post(int message_index, int bit_size, int bit_pos, uint64_t value);

template <typename T, typename buf_t>
T __lookup4(int message_index, int bit_size, int bit_pos) {
    buf_t buf = (msg[message_index] >> bit_pos) & (bit_size - 1);
    return *((T *)&buf);
}

template <typename T, typename buf_t, typename calc_t>
T __lookup4_s(int message_index, int bit_size, int bit_pos, calc_t scale, calc_t offset) {
    buf_t buf = ((msg[message_index] >> bit_pos) & (bit_size - 1));
    return *((T *)&buf) * scale + offset;
}

#define __BIT_TYPE(bits) uint##bits##_t

#define __lookup3(i, sz, pos, post_t, buf_t, ...) __lookup4<post_t, buf_t>(i, sz, pos)
#define __lookup3_s(i, sz, pos, post_t, buf_t, scale, offset, calc_t) __lookup4_s<post_t, buf_t, calc_t>(i, sz, pos, scale, offset)
#define __lookup2(i, sz, pos, post_t, bits, scale, offset, calc_t, ver) __lookup3##ver(i, sz, pos, post_t, __BIT_TYPE(bits), scale, offset, calc_t)
#define __lookup1(i, sz, pos, meta) __lookup2(i, sz, pos, meta)
#define lookup(post_id) __lookup1(post_id, post_id##_META)

void test() {
    int a = lookup(POST_WHEEL_SPEED_FRONT_LEFT);
    a++;
    post(POST_WHEEL_SPEED_FRONT_LEFT, a);
}