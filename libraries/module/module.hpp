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

using bitmapVal_t = unsigned int;

static const size_t maxModules = sizeof(bitmapVal_t) * 8;

class Module_t;

static bitmapVal_t s_id = 0;

class Module_t {
private:
    std::mutex vMux;
    int thread = -1;

    void start();
    void stop();

    static void _runner(Module_t *m) {
        Log.i(ID, "Starting Runner", m->id);
        m->runner();
    };

public:
    const bitmapVal_t count = 0;
    const Module_t **dependents = nullptr;
    const bitmapVal_t id;

    Module_t() : id(1 << s_id++){};
    Module_t(const Module_t **dependents, const bitmapVal_t count) : count(count), dependents(dependents), id(1 << s_id++){};

    virtual void print();
    virtual void setup() = 0;
    virtual void runner(){};

    static void startModules(Module_t **modules, bitmapVal_t count);
    static void restartModules(Module_t **modules, bitmapVal_t count);
    static void stopModules(Module_t **modules, bitmapVal_t count);
    static void printModules(Module_t **modules, bitmapVal_t count);
    static void setupModules(Module_t **modules, bitmapVal_t count);
};

} // namespace Module

#endif // __MODULE_HPP__