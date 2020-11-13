#ifndef __ECU_MESSAGES_HPP__
#define __ECU_MESSAGES_HPP__

#include "CanbusConfig.def"
#include "FlexCAN_T4.h"
#include "WProgram.h"

namespace CanbusMsg {

void BMSFaultMsg(const CAN_message_t &msg);
void RMSFaultMsg(const CAN_message_t &msg);

}

#endif // __ECU_MESSAGES_HPP__