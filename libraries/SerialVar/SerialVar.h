/**
 * @file SerialVar.h
 * @author IR
 * @brief Serial Variable Module
 * @version 0.1
 * @date 2021-11-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ECU_SERIALVAR_H__
#define __ECU_SERIALVAR_H__

#include "SerialVars.def"
#include "wiring.h"

/**
 * @brief SerialVar is used for variables that can be modified over usb serial while still being used during runtime like a normal variable
 */
namespace SerialVar {

/**
 * @brief Generalization of all serial variable types
 */
class SerialVarObj {
public:
    virtual void update(byte *dataArr);
};

/**
 * @brief Template of a Serial Variable
 * 
 * @tparam T The primitive data type of this variable
 * @tparam ID The unique ID of this variable, used to update it in the background
 * @note Each instance of a SerialVar **must** be defined in SerialVars.def, where you can then declare in instance of SerialVar as so.
 * ``` c++
 * SerialVar<SERIALVAR_TORQUE_VECTORING_AGGRESSION> TVA;
 * ```
 */
template <typename T>
class SerialVar : public SerialVarObj {
private:
    volatile T val = 0;

public:
    virtual void update(byte *dataArr) {
        SerialVar s(*this);
        s.val = *((T *)dataArr);
    }

    operator T() const { return this->val; }
};

// The largest ID number defined
static const size_t MAX_ID = (
#define X(type, ID) max(ID,
    SERIALVARS
#undef X
    - 1
#define X(type, ID) )
    SERIALVARS
#undef X
);

static SerialVarObj variables[MAX_ID + 1] = {
#define X(type, ID) SerialVar<type>(),
    SERIALVARS};
#undef X

/**
 * @brief Get the Serial Variable for a given ID
 * 
 * @param ID the ID of the variable
 * @return SerialVarObj 
 */
SerialVarObj getVariable(size_t ID);

} // namespace SerialVar
#endif // __ECU_SERIALVAR_H__