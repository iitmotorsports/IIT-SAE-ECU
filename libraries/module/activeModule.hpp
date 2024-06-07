#pragma once

#include "mainModule.hpp"

namespace Module {

static const int DEFAULT_STACK_SIZE = 4096;

class ActiveModule_t : public Module::Module_t {
private:
    std::mutex vMux;
    int threadID = -1;
    const int stackSize = 0;

    void start();
    void stop();

    static void _runner(ActiveModule_t *m);

    friend Manager_t;

protected:
    virtual void run();

public:
    static const int classID = 1;

    virtual int getClassID() const { return classID; }

    template <typename... T>
    ActiveModule_t(const int stackSize) : stackSize(stackSize), Module_t(){};
    template <typename... T>
    ActiveModule_t(const int stackSize, T *...mods) : stackSize(stackSize), Module_t(mods...){};
    // ActiveModule_t(int stackSize, T *...mods) : stackSize(stackSize), count(sizeof...(mods)), dependents{static_cast<Module_t *>(mods)...}, id(1 << s_id++) {  };
};

} // namespace Module