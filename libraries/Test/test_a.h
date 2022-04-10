#ifndef __TEST_A_H__
#define __TEST_A_H__

#include "module.hpp"
#include "test_b.h"

class a : public Module::Module_t {
    using Module::Module_t::Module_t;

    LOG_TAG ID = "a";

    volatile bool state = false;

    void setup() {
        Log.i(ID, "Setup", id);
    }

    volatile int counter = 0;
    volatile bool led = false;

    void runner() {
        while (1) {
            if ((state = !state)) {
                Log.i(ID, "running", counter++);
                threads.delay(500);
                Log.w(ID, "LED!");
                Pins::setPinValue(PINS_BOTH_LED, (led = !led));
            } else {
                Log.i(ID, "running alt");
                threads.delay(1000);
            }
        }
    }

    void print() {
        Log.i(ID, "ID", id);
    }

} a(&b, &c, &d);

#endif // __TEST_A_H__