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

#include "Log.h"
#include "SerialVars.def"
#include "WProgram.h"

/**
 * @brief SerialVar is used for variables that can be modified over usb serial while still being used during runtime like a normal variable
 */
namespace SerialVar {

/**
 * @brief Get the Serial Variable for a given ID
 * 
 * @param ID the ID of the variable
 * @return SerialVarObj_t 
 */
uint8_t *getVariable(size_t ID);

/**
 * @brief Receive a variable over serial
 */
void receiveSerialVar();

/**
 * @brief Template of a Serial Variable
 * 
 * @tparam T The primitive data type of this variable
 * @tparam ID The unique ID of this variable, used to update it in the background
 * @note Each instance of a SerialVar **must** be defined in SerialVars.def, where you can then declare in instance of SerialVar as so.
 * ``` c++
 * SerialVar::SerialVar<float> TVAggression = SerialVar::getVariable(SERIALVAR_TORQUE_VECTORING_AGGRESSION);
 * ```
 */
template <typename T>
class SerialVar {
private:
    uint8_t *buffer;

public:
    /**
     * @brief Construct a new Serial Var given the predefined ID defined in SerialVars.def
     * 
     * @param ID the predefined ID
     */
    SerialVar(int ID) {
        buffer = getVariable(ID);
    }

    /**
     * @brief Cast to the given primitive data type of this variable
     * 
     * @return T The buffer interpreted as the primitive data type of this variable
     */
    operator T() const { return *((T *)(buffer)); }

    /**
     * @brief Set this variable's buffer given the primitive data type T
     * 
     * @param val Value to interpret
     * @return SerialVar& pointer to this object
     */
    SerialVar &operator=(T val) {
        *((T *)(buffer)) = val;
        return *this;
    }
};

/**
 * @brief Convienece type for SerialVar floats
 */
typedef SerialVar<float> SerialFloat;
/**
 * @brief Convienece type for SerialVar ints
 */
typedef SerialVar<int> SerialInt;
/**
 * @brief Convienece type for SerialVar uints
 */
typedef SerialVar<unsigned int> SerialUInt;

} // namespace SerialVar
#endif // __ECU_SERIALVAR_H__