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

/**
 * @brief Variable that automaticaly averages itself out when it is set to a value
 * 
 * @note The underlying implementation converts everything to a double to maximize precision when averaging
 * 
 * @tparam T The primitive data type of this variable
 */
template <typename T>
class AvgVar {
private:
    double avg = 0.0;
    int samples;

public:
    AvgVar(int samples) {
        this->samples = samples;
    }

    operator T() const { return (T)avg; }

    T operator=(T val) {
        avg = EMAvg(avg, val, samples);
        return (T)avg;
    }
};

/**
 * @brief Variable that automaticaly averages itself out when it is called, that is, it only updates it's average when the variable is used
 * 
 * @note The underlying implementation converts everything to a double to maximize precision when averaging
 * 
 * @tparam T The primitive data type of this variable
 */
template <typename T>
class AvgVarRef {
private:
    double avg = 0.0;
    T *pointer;
    int samples;

public:
    AvgVarRef(T *pointer, int samples) {
        this->pointer = pointer;
        this->samples = samples;
    }

    operator T() {
        avg = EMAvg(avg, *pointer, samples);
        return (T)avg;
    }
};

#endif // __ECU_UTIL_H__