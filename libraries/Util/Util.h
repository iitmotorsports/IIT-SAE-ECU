/**
 * @file Util.h
 * @author IR
 * @brief Various utility functions
 * @version 0.1
 * @date 2021-10-13
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ECU_UTIL_H__
#define __ECU_UTIL_H__

#define Euler exp(1.0)
#define clamp(v, m, x) min(max(v, m), x)

/**
 * @brief Exponential moving average
 * 
 * @param lastVal The last value that was returned
 * @param newVal The new value that should be added to the average
 * @param memCount The total number of *samples* to be averaged
 * @return double The EMA of the last memCount values
 */
double EMAvg(double lastVal, double newVal, int memCount);

/**
 * @brief Map a value from one range to another while clamping the value to boundaries
 * 
 * @param x The value to be mapped
 * @param inMin The minimum input value
 * @param inMax The maximum input value
 * @param outMin The minimum output value
 * @param outMax The maximum output value
 * @return The mapped value for x
 */
template <class T, class A, class B, class C, class D>
T cMap(T x, A inMin, B inMax, C outMin, D outMax) {
    T mapped = map(x, inMin, inMax, outMin, outMax);
    if (mapped < outMin)
        return outMin;
    if (mapped > outMax)
        return outMax;
    return mapped;
}

#endif // __ECU_UTIL_H__