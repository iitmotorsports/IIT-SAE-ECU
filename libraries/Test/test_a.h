#ifndef __TEST_A_H__
#define __TEST_A_H__

#include "module.hpp"
#include "test_b.h"

class a : public Module::Module_t {

    LOG_TAG ID = "a";

    volatile bool state = false;

    static const int dCount = 2;
    const Module_t *deps[dCount] = {&b, &c};

    void setup() {
        Log.i(ID, "Setup", id);
    }

    volatile int counter = 0;

    void runner() {
        while (1) {
            if ((state = !state)) {
                Log.i(ID, "running", counter++);
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

#endif // __TEST_A_H__