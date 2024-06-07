/**
 * @file Util.cpp
 * @author IR
 * @brief Util source file
 * @version 0.1
 * @date 2022-01-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "Util.h"

double EMAvg(double lastVal, double newVal, int memCount) {
    return (1.0 - (1.0 / memCount)) * lastVal + (1.0 / memCount) * newVal;
}