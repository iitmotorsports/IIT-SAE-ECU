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

void Module_t::start() {
    std::lock_guard<std::mutex> lock(vMux);
    if (thread == -1) {
        thread = threads.addThread((ThreadFunction)_runner, (void *)this, 4096);
    }
}

void Module_t::stop() {
    std::lock_guard<std::mutex> lock(vMux);
    if (thread != -1) {
        threads.kill(thread);
        threads.wait(thread);
        thread = -1;
    }
}

void Module_t::print() {
    Log.i(ID, "ID", id);
}

bool Module_t::setupModules(Module_t **modules, bitmapVal_t count) {
    Log.i(ID, "Ordering Modules");
    if (orderModules(modules, count)) {
        for (bitmapVal_t i = 0; i < count; i++) {
            Module_t *m = modules[i];
            if (m == 0)
                break;
            m->print();
            m->setup();
        }
        return true;
    }
    return false;
}

void Module_t::startModules(Module_t **modules, bitmapVal_t count) {
    std::lock_guard<std::mutex> lock(uMux);
    if (setupModules(modules, count)) {
        Log.i(ID, "Starting Modules");
        for (bitmapVal_t i = 0; i < count; i++) {
            Module_t *m = modules[i];
            if (m == 0)
                break;
            m->start();
        }
    }
}

void Module_t::stopModules(Module_t **modules, bitmapVal_t count) {
    Log.w(ID, "Stopping Modules");
    std::lock_guard<std::mutex> lock(uMux);
    for (bitmapVal_t i = 0; i < count; i++) {
        Module_t *m = modules[i];
        if (m == 0)
            break;
        m->stop();
    }
}

void Module_t::restartModules(Module_t **modules, bitmapVal_t count) {
    stopModules(modules, count);
    startModules(modules, count);
}

void Module_t::printModules(Module_t **modules, bitmapVal_t count) {
    for (bitmapVal_t i = 0; i < count; i++) {
        Module_t *m = modules[i];
        if (m == 0)
            break;
        m->print();
    }
}

} // namespace Module