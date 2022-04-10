#include "module.hpp"

class c : public Module::Module_t {

    LOG_TAG ID = "c";

    void setup() {
        Log.i(ID, "Setup", id);
    }

    void runner() {
        while (1) {
            Log.i(ID, "running");
            threads.delay(1000);
        }
    }

    void print() {
        Log.i(ID, "ID", id);
    }
} c;

class b : public Module::Module_t {

    LOG_TAG ID = "b";

    const Module_t *deps[1] = {&c};

public:
    b() : Module_t(deps, 1) {}

    void setup() {
        Log.i(ID, "Setup", id);
    }

    void runner() {
        while (1) {
            Log.i(ID, "running");
            threads.delay(1000);
        }
    }

    void print() {
        Log.i(ID, "ID", id);
    }
} b;

class a : public Module::Module_t {

    LOG_TAG ID = "a";

    static const int dCount = 2;
    const Module_t *deps[dCount] = {&b, &c};

public:
    a() : Module_t(deps, dCount) {}

    void setup() {
        Log.i(ID, "Setup", id);
    }

    void runner() {
        while (1) {
            Log.i(ID, "running");
            threads.delay(1000);
        }
    }

    void print() {
        Log.i(ID, "ID", id);
    }
} a;

Module::Module_t *modules[] = {&a, &b, &c};

void testModules() {
    Log.i("Main", "Starting");
    Module::Module_t::startModules(modules, sizeof(modules) / sizeof(modules[0]));

    threads.addThread([](void *) {
        while (1) {
            Log.d("test", "test");
            threads.delay(100);
        } }, 0, 2048);

    while (1) {
        Log.i("Main", "Waiting");
        threads.delay(5000);
    }
}