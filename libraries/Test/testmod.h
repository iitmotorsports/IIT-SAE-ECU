#include "module.hpp"

#include "test_a.h"
#include "test_b.h"

void testModules() {
    Log.i("Main", "Starting");
    threads.setSliceMillis(5);
    Module::Module_t::startModules();

    threads.addThread([](void *) {
        while (1) {
            Log.d("test", "test");
            threads.delay(100);
        } }, 0, 2048);

    while (1) {
        Log.i("Main", "Waiting");
        threads.delay(10000);
        Module::Module_t::restartModules();
    }
}