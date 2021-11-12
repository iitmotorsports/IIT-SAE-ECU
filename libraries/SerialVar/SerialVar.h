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
    SerialVar(int ID) {
        buffer = getVariable(ID);
    }

    operator T() const { return *((T *)(buffer)); }

    SerialVar &operator=(T val) {
        *((T *)(buffer)) = val;
        return *this;
    }
};

typedef SerialVar<float> SerialFloat;
typedef SerialVar<int> SerialInt;
typedef SerialVar<unsigned int> SerialUInt;

} // namespace SerialVar
#endif // __ECU_SERIALVAR_H__