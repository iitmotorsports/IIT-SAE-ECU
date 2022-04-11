#ifndef __TEST_B_H__
#define __TEST_B_H__

#include "module.hpp"

#include "Pins.h"

class e : public Module::Module_t {

    LOG_TAG ID = "e";

    void setup() {
        Log.i(ID, "Setup", id);
        Pins::initialize();
    }

    volatile int counter = 0;

    void runner() {
        while (1) {
            Log.i(ID, "running", counter++);
            threads.delay(350);
            Log.w(ID, "LED!");
            Pins::setPinValue(PINS_BOTH_LED, counter % 2);
        }
    }

    void print() {
        Log.i(ID, "ID", id);
    }
} e;

class d_t : public Module::Module_t {

    using Module::Module_t::Module_t;

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

} d("d", &e);

class c_t : public Module::Module_t {

    using Module::Module_t::Module_t;

    LOG_TAG ID = "c";

    void setup() {
        Log.i(ID, "Setup", id);
    }

    volatile int counter = 0;

    // void runner() {
    //     while (1) {
    //         Log.i(ID, "running", counter++);
    //         threads.delay(1000);
    //     }
    // }

    void print() {
        Log.i(ID, "ID", id);
    }

} c("c", &d);

class b_t : public Module::Module_t {

    using Module::Module_t::Module_t;

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
} b("b");

d_t d0("d0", &b);
#endif // __TEST_B_H__