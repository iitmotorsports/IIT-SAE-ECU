#include "Util.h"

double EMAvg(double lastVal, double newVal, int memCount) {
    return (1.0 - (1.0 / memCount)) * lastVal + (1.0 / memCount) * newVal;
}
