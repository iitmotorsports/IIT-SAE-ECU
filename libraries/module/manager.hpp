#pragma once

namespace Module {

class Manager_t {
    static bool setupModules();
    static bool orderModules();
    static void startModules();
    static void stopModules();
    static void restartModules();
    static void printModules();
    static void start();
} ModuleManager;

} // namespace Module
