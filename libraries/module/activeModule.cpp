#include "activeModule.hpp"

namespace Module {

void ActiveModule_t::_runner(ActiveModule_t *m) {
    Log.d(ID, "Starting thread", m->id);
    m->run();
    Log.d(ID, "Thread stopped", m->id);
};

void ActiveModule_t::start() {
    std::lock_guard<std::mutex> lock(vMux);
    if (threadID == -1) {
        threadID = Thread::addThread((Thread::ThreadFunction)_runner, (void *)this, stackSize, 0);
        if (threadID == -1) {
            Log.f(ID, "Failed to start thread", id);
        }
    }
}

void ActiveModule_t::stop() {
    std::lock_guard<std::mutex> lock(vMux);
    if (threadID != -1) {
        Thread::kill(threadID);
        Thread::wait(threadID);
        threadID = -1;
    }
}

} // namespace Module