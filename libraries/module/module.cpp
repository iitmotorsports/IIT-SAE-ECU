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

#include "module.hpp"
#include "moduleOrder.hpp"

static std::mutex uMux;

namespace Module {

bitmapVal_t s_id;

Module_t *allModules[maxModules];
static bool modulesOrdered = false;

void Module_t::_runner(Module_t *m) {
    Log.i(ID, "Starting thread", m->id);
    m->runner();
};

void Module_t::start() {
    std::lock_guard<std::mutex> lock(vMux);
    if (hasRunner && thread == -1) {
        thread = Thread::addThread((Thread::ThreadFunction)_runner, (void *)this, stackSize, 0, name);
        if (thread == -1) {
            Log.f(ID, "Failed to start thread", id);
        }
    }
}

void Module_t::stop() {
    std::lock_guard<std::mutex> lock(vMux);
    if (thread != -1) {
        Thread::kill(thread);
        Thread::wait(thread);
        thread = -1;
    }
}

void Module_t::print() {
    Log.i(ID, "ID", id);
}

bool Module_t::setupModules() {
    if (modulesOrdered)
        return modulesOrdered;
    Log.i(ID, "Ordering modules", s_id);
    if (orderModules()) {
        for (bitmapVal_t i = 0; i < s_id; i++) {
            Module_t *m = allModules[i];
            if (m == 0)
                break;
            m->setup();
        }
        modulesOrdered = true;
    }
    return modulesOrdered;
}

void Module_t::startModules() {
    std::lock_guard<std::mutex> lock(uMux);
    if (setupModules()) {
        Log.w(ID, "Starting modules");
        for (bitmapVal_t i = 0; i < s_id; i++) {
            Module_t *m = allModules[i];
            if (m == 0)
                break;
            m->start();
        }
    }
}

void Module_t::stopModules() {
    std::lock_guard<std::mutex> lock(uMux);
    Log.w(ID, "Stopping modules");
    for (bitmapVal_t i = 0; i < s_id; i++) {
        Module_t *m = allModules[i];
        if (m == 0)
            break;
        m->stop();
    }
}

void Module_t::restartModules() {
    stopModules();
    startModules();
}

void Module_t::printModules() {
    for (bitmapVal_t i = 0; i < s_id; i++) {
        Module_t *m = allModules[i];
        if (m == 0)
            break;
        m->print();
    }
}

} // namespace Module