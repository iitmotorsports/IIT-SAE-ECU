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

#include "Log.h"
#include "TeensyThreads.h"

namespace Module {

static LOG_TAG ID = "Module";

using bitmapVal_t = unsigned short;

static const size_t maxModules = sizeof(bitmapVal_t) * 8;

class Module_t;

extern bitmapVal_t s_id;

extern Module_t *allModules[maxModules];

class Module_t {
private:
    std::mutex vMux;
    int thread = -1;

    const bitmapVal_t count = 0;
    const Module_t *dependents[maxModules] = {0}; // IMPROVE: make flexible dependency array

    void start();
    void stop();

    static void _runner(Module_t *m) {
        Log.i(ID, "Starting Runner", m->id);
        m->runner();
    };

    static bool setupModules();
    static bool orderModules();

protected:
    const bitmapVal_t id;

    virtual void print();
    virtual void setup() = 0;
    virtual void runner(){};

public:
    template <typename... T>
    Module_t(T *...mods) : count(sizeof...(mods)), dependents{static_cast<Module_t *>(mods)...}, id(1 << s_id++) { allModules[s_id - 1] = this; };

    static void startModules();
    static void restartModules();
    static void stopModules();
    static void printModules();
};

} // namespace Module

#endif // __MODULE_HPP__