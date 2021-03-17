/**
 * @file Command.h
 * @author IR
 * @brief Serial read functionality
 * @version 0.1
 * @date 2021-03-16
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ECU_COMMAND_H__
#define __ECU_COMMAND_H__

#include <stdint.h>
#include <stdlib.h>

#define COMMAND_ENABLE_CHARGING 186
#define COMMAND_SEND_CANBUS_MESSAGE 218
#define COMMAND_CLEAR_FAULTS 379
#define COMMAND_TOGGLE_CANBUS_SNIFF 492
#define COMMAND_PING 518

#endif // __ECU_COMMAND_H__