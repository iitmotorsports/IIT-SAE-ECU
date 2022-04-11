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

static LOG_TAG ID = "Module";

using bitmapVal_t = unsigned short;

class Module_t;

extern bitmapVal_t s_id;

static const size_t maxModules = sizeof(bitmapVal_t) * 8;

extern Module_t *allModules[maxModules];

class Module_t { // NOTE: Modules are only meant to be created staticly
private:
    std::mutex vMux;
    int thread = -1;
    bool hasRunner = true; // TODO: set to false if no runner is overriden

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
    const bitmapVal_t id;

    virtual void print();
    virtual void setup() = 0;
    virtual void runner(){};

public:
    template <typename... T>
    Module_t(T *...mods) : count(sizeof...(mods)), dependents{static_cast<Module_t *>(mods)...}, id(1 << s_id++) { allModules[s_id - 1] = this; };

    static void startManager() {
        static LOG_TAG ID = "Module Manager";
        Log.i(ID, "Starting");
        startModules();
        while (1) {
#if CONF_LOGGING_ASCII_DEBUG
            Serial.println(threads.threadsInfo());
#endif
            threads.delay(500);
        }
    }
};

} // namespace Module

#endif // __MODULE_HPP__