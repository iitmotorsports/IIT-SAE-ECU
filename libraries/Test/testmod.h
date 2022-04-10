#include "module.hpp"

#include "allmods.h"

void testModules() {
    Log.i("Main", "Starting");
    threads.setSliceMillis(5);
    Module::Module_t::startModules(modules, sizeof(modules) / sizeof(modules[0]));

    threads.addThread([](void *) {
        while (1) {
            Log.d("test", "testing");
            threads.delay(100);
        } }, 0, 2048);

    while (1) {
        Log.i("Main", "Waiting");
        threads.delay(10000);
        Module::Module_t::restartModules(modules, sizeof(modules) / sizeof(modules[0]));
    }
}