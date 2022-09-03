/**
 * AUTO GENERATED FILE - DO NOT EDIT
 *
 * SDBC Values, defined in C as their respective ID
 */

#include <stdint.h>

uint32_t f_rpm__in(uint32_t data) {
    return data;
}

float deg__in(uint32_t data) {
    float f_data = *(float *)(&data);
    return f_data;
}

uint32_t deg__out(float value) {
    uint32_t data = *(uint32_t *)(&value);
    return data;
}
