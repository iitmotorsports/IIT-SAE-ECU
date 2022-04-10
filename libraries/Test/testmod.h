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

    volatile  bool state = false;

    static const int dCount = 2;
    const Module_t *deps[dCount] = {&b, &c};

    void setup() {
        Log.i(ID, "Setup", id);
    }

    void runner() {
        while (1) {
            if ((state = !state)) {
                Log.i(ID, "running");
                threads.delay(500);
            } else {
                Log.i(ID, "running alt");
                threads.delay(1000);
            }
        }
    }

    void print() {
        Log.i(ID, "ID", id);
    }

public:
    a() : Module_t(deps, dCount) {}
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