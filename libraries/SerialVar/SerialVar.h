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

LOG_TAG LOGID = "SerialVar";

/**
 * @brief Generalization of all serial variable types
 */
class SerialVarObj_t {
public:
    /**
     * @brief Header for a SerialVar's update function
     * 
     * @param dataArr The data array to be interpreted
     * @param count The size of this array to ensure it matches the data type size
     */
    void update(byte *dataArr, int count);
};

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
class SerialVar : public SerialVarObj_t {
private:
    volatile T val;

public:
    static const int byteCount = sizeof(T);
    SerialVar() {}
    SerialVar(SerialVarObj_t obj) {}

    /**
     * @brief Update function which is used internally. Ensures the data array size to be interpreted is the correct length
     * 
     * @param dataArr The data array to be interpreted
     * @param count The size of this array to ensure it matches the data type size
     */
    void update(byte *dataArr, int count) {
        if (count < byteCount) {
            Log.w(LOGID, "The given data chunk is not big enough for this data type, variable not updated", byteCount);
            return;
        }
        SerialVar s(*this);
        s.val = *((T *)dataArr);
    }

    operator T() const { return this->val; }

    SerialVar &operator=(T val) {
        this->val = val;
        return *this;
    }
};

/**
 * @brief Get the Serial Variable for a given ID
 * 
 * @param ID the ID of the variable
 * @return SerialVarObj_t 
 */
SerialVarObj_t getVariable(size_t ID);

/**
 * @brief Receive a variable over serial
 */
void receiveSerialVar();

} // namespace SerialVar
#endif // __ECU_SERIALVAR_H__