/**
 * @file module.cpp
 * @author IR
 * @brief
 * @version 0.1
 * @date 2022-04-07
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "mainModule.hpp"

namespace Module {

void Module_t::print() {
    Log.d(ID, "ID", id);
}

} // namespace Module