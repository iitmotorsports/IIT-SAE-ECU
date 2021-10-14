#include "Util.h"

double lerp(double t, double i, double f, double d) {
    // quad
    t = t / (d * 2);
    if (t < 1) {
        return f / 2 * t * t + i;
    }
    return -f / 2 * ((t - 1) * (t - 3) - 1) + i;

    //circular
    // t = t / (d * 2);
    // if (t < 1) {
    //     return -f / 2 * (sqrt(1 - t * t) - 1) + i;
    // }
    // t = t - 2;
    // return f / 2 * (sqrt(1 - t * t) + 1) + i;

    // quad rev
    // if (t < d / 2) {
    //     return -f / 2 * (pow(t * 2 / d - 1, 4) - 1) + i;
    // }
    // return (f / 2) * pow(((t * 2) - d) / d, 4) + (i + f / 2);
}