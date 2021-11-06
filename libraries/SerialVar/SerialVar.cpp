/**
 * @file SerialVar.cpp
 * @author IR
 * @brief SerialVar Source File
 * @version 0.1
 * @date 2021-11-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

// @cond

#include "SerialVar.h"
#include "PPHelp.h"
#include "wiring.h"

namespace SerialVar {

SerialVarObj getVariable(size_t ID) {
    return variables[ID];
}

void updateVariable(size_t ID, byte *dataArr) {
    variables[ID].update(dataArr);
}

} // namespace SerialVar
  // @endcond