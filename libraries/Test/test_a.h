#ifndef __TEST_A_H__
#define __TEST_A_H__

#include "module.hpp"
#include "test_b.h"

class a : public Module::Module_t {
    LOG_TAG ID = "a";

    using Module::Module_t::Module_t;

    void setup() {
        Log.i(ID, "Setup", id);
    }

    volatile bool state = false;
    volatile int counter = 0;
    volatile bool led = false;

    void runner() {
        while (1) {
            if ((state = !state)) {
                Log.i(ID, "running", counter++);
                Thread::delay(500);
                Log.w(ID, "LED!");
                Pins::setPinValue(PINS_BOTH_LED, (led = !led));
            } else {
                Log.i(ID, "running alt");
                Thread::delay(1000);
            }
        }
    }

} a("a", 512, &b, &c, &d);

#endif // __TEST_A_H__