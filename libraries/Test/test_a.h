#ifndef __TEST_A_H__
#define __TEST_A_H__

#include "module.hpp"
#include "test_b.h"

class GPIO : public Module::MessengerModule_t {
    LOG_TAG ID = "GPIO";

    using Module::MessengerModule_t::MessengerModule_t;

    void setup() {
        Log.i(ID, "Setup", id);
    }

    volatile bool state = false;
    volatile int counter = 0;
    volatile bool led = false;

    void run() {
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

} GPIO(Module::DEFAULT_STACK_SIZE);

class a : public Module::MessengerModule_t {
    LOG_TAG ID = "a";

    using Module::MessengerModule_t::MessengerModule_t;

    void setup() {
        Log.i(ID, "Setup", id);
    }

    volatile bool state = false;
    volatile int counter = 0;
    volatile bool led = false;

    void run() {
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

} a(Module::DEFAULT_STACK_SIZE, &b, &c, &d);

#endif // __TEST_A_H__