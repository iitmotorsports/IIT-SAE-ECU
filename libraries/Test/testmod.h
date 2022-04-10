#include "module.hpp"

#include "test_a.h"
#include "test_b.h"

void testModules() {
    Log.i("Main", "Starting");

    threads.addThread([](void *) {
        while (1) {
            Log.d("test", "test");
            threads.delay(100);
        } }, 0, 2048);

    Module::Module_t::startManager();
}