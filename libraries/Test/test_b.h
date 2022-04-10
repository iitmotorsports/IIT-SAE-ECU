#ifndef __TEST_B_H__
#define __TEST_B_H__

#include "module.hpp"

class e : public Module::Module_t {

    LOG_TAG ID = "e";

    void setup() {
        Log.i(ID, "Setup", id);
    }

    volatile int counter = 0;

    void runner() {
        while (1) {
            Log.i(ID, "running", counter++);
            threads.delay(350);
        }
    }

    void print() {
        Log.i(ID, "ID", id);
    }
} e;

class d : public Module::Module_t {

    LOG_TAG ID = "d";

    void setup() {
        Log.i(ID, "Setup", id);
    }

    volatile int counter = 0;

    void runner() {
        while (1) {
            Log.i(ID, "running", counter++);
            threads.delay(1000);
            if (counter == 20) {
                counter /= 4;
            }
        }
    }

    void print() {
        Log.i(ID, "ID", id);
    }

    static const int dCount = 1;
    const Module_t *deps[dCount] = {&e};

public:
    d() : Module_t(deps, dCount) {}
} d;

class c : public Module::Module_t {

    LOG_TAG ID = "c";

    void setup() {
        Log.i(ID, "Setup", id);
    }

    volatile int counter = 0;

    void runner() {
        while (1) {
            Log.i(ID, "running", counter++);
            threads.delay(1000);
        }
    }

    void print() {
        Log.i(ID, "ID", id);
    }

    static const int dCount = 1;
    const Module_t *deps[dCount] = {&d};

public:
    c() : Module_t(deps, dCount) {}
} c;

class b_t : public Module::Module_t {

    LOG_TAG ID = "b";

    void setup() {
        Log.i(ID, "Setup", id);
    }

    volatile int counter = 0;

    void runner() {
        while (1) {
            Log.i(ID, "running", counter++);
            threads.delay(1000);
        }
    }

    void print() {
        Log.i(ID, "ID", id);
    }
} b;
#endif // __TEST_B_H__