/**
 * @file module.hpp
 * @author IR
 * @brief
 * @version 0.1
 * @date 2022-04-07
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef __MODULE_HPP__
#define __MODULE_HPP__

#include "stdlib.h"
#include <queue>
#include <variant>

#include "Log.h"
#include "TeensyThreads.h"

namespace Module {

class Module_t;
class Manager_t;
using bitmapVal_t = unsigned short;

const LOG_TAG ID = "Module";
const size_t maxModules = sizeof(bitmapVal_t) * 8;

extern bitmapVal_t s_id;
extern Module_t *allModules[maxModules];

/**
 * @brief Module_t should be used for major components of a framework, it is used to isolate, thread, and handle dependencies between these components
 *
 * Defining a module is not straight forward and needs to be setup in a specific way. Reference the example.
 *
 * Example definition:
 *
 * >class mod_name : public Module::Module_t {
 * >    LOG_TAG ID = "String ID for logging";
 * >
 * >    using Module::Module_t::Module_t;
 * >
 * >    void setup() {
 * >        // Setup code goes here, this includes anything that should be reset or initalized
 * >        // Called on startup and whenever the ecu soft resets
 * >    }
 * >
 * >    // Only runs after all modules run their setup. Loop is managed by user.
 * >    void run() {
 * >        while (1) {
 * >            // Looping code goes here
 * >        }
 * >    }
 * >
 * >} a("String ID for logging", 2048, &b, &c, &d); // The EXACT same string as the ID,
 * >                                                // optionally, the stacksize this module should allocate,
 * >                                                // and pointers to all the modules that this module depends on
 *
 * @warning Modules are only meant to be created staticly as shown above
 */
class Module_t {
private:
    const bitmapVal_t count = 0;
    const Module_t *dependents[maxModules] = {0}; // IMPROVE: make flexible dependency array

    friend Manager_t;

protected:
    const bitmapVal_t id;

    virtual void print();
    virtual void setup();

public:
    static const int classID = 0;
    virtual int getClassID() const { return classID; }

    Module_t() : id(1 << s_id++) { allModules[s_id - 1] = this; };
    template <typename... T>
    Module_t(T *...mods) : count(sizeof...(mods)), dependents{static_cast<Module_t *>(mods)...}, id(1 << s_id++) { allModules[s_id - 1] = this; };
};

} // namespace Module

#endif // __MODULE_HPP__