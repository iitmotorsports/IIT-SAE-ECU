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

#include "Log.h"
#include "TeensyThreads.h"

namespace Module {

static const LOG_TAG ID = "Module";

using bitmapVal_t = unsigned short;

class Module_t;

extern bitmapVal_t s_id;

static const size_t maxModules = sizeof(bitmapVal_t) * 8;

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
    std::mutex vMux;
    int thread = -1;
    const int stackSize = 4096;
    bool hasRun = true; // TODO: set to false if no run is overriden

    const bitmapVal_t count = 0;
    const Module_t *dependents[maxModules] = {0}; // IMPROVE: make flexible dependency array

    void start();
    void stop();

    static void _runner(Module_t *m);

    static bool setupModules();
    static bool orderModules();
    static void startModules();
    static void stopModules();
    static void restartModules();
    static void printModules();

protected:
    const char *name;
    const bitmapVal_t id;

    virtual void print();
    virtual void setup() = 0;
    virtual void run(){};

public:
    Module_t() : name(Thread::NIL_NAME), id(1 << s_id++) { allModules[s_id - 1] = this; };
    template <typename... T>
    Module_t(const char *name, T *...mods) : count(sizeof...(mods)), dependents{static_cast<Module_t *>(mods)...}, name(name), id(1 << s_id++) { allModules[s_id - 1] = this; };
    template <typename... T>
    Module_t(const char *name, int stackSize, T *...mods) : stackSize(stackSize), count(sizeof...(mods)), dependents{static_cast<Module_t *>(mods)...}, name(name), id(1 << s_id++) { allModules[s_id - 1] = this; };

    static void startManager() {
        Log.i("Module Manager", "Starting");
        startModules();
        while (1) {
#if CONF_LOGGING_ASCII_DEBUG
            Serial.println(Thread::infoString());
#endif
            Thread::delay(500);
        }
    }
};

} // namespace Module

#endif // __MODULE_HPP__