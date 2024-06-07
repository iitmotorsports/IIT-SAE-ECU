#include "manager.hpp"
#include "messageModule.hpp"

namespace Module {

static bool modulesOrdered = false;
static std::mutex uMux;
bitmapVal_t s_id;
Module_t *allModules[maxModules];

// Should we be allowed to re-setup modules?
bool Manager_t::setupModules() {
    if (modulesOrdered)
        return modulesOrdered;
    Log.d(ID, "Ordering modules", s_id);
    if (orderModules()) {
        for (bitmapVal_t i = 0; i < s_id; i++) {
            Module_t *m = allModules[i];
            if (m == 0)
                break;
            m->setup();
        }
        modulesOrdered = true;
    } else {
        Log.e(ID, "Failed to order modules");
    }
    return modulesOrdered;
}

void Manager_t::startModules() {
    std::lock_guard<std::mutex> lock(uMux);
    if (setupModules()) {
        Log.d(ID, "Starting modules");
        for (bitmapVal_t i = 0; i < s_id; i++) {
            Module_t *m = allModules[i];
            if (m == 0) {
                break;
            } else if (m->getClassID() == ActiveModule_t::classID) {
                ActiveModule_t *am = static_cast<ActiveModule_t *>(m);
                if (!am->stackSize) {
                    Log.d(ID, "Not running module, stack size 0", i);
                    continue;
                }
                am->start();
            }
        }
    } else {
        Log.e(ID, "Failed to setup modules");
    }
}

void Manager_t::stopModules() {
    std::lock_guard<std::mutex> lock(uMux);
    Log.d(ID, "Stopping modules");
    for (bitmapVal_t i = 0; i < s_id; i++) {
        Module_t *m = allModules[i];
        if (m == 0) {
            break;
        } else if (m->getClassID() == ActiveModule_t::classID) {
            ActiveModule_t *am = static_cast<ActiveModule_t *>(m);
            am->stop();
        }
    }
}

void Manager_t::restartModules() {
    stopModules();
    startModules();
}

void Manager_t::printModules() {
    for (bitmapVal_t i = 0; i < s_id; i++) {
        Module_t *m = allModules[i];
        if (m == 0)
            break;
        m->print();
    }
}

void Manager_t::start() {
    Log.d(ID, "Starting");
    startModules();
    Log.d(ID, "Starting Loop");
    while (1) {
#if CONF_LOGGING_ASCII_DEBUG
        Serial.println(Thread::infoString());
#endif
        Thread::delay(500);
    }
}
} // namespace Module