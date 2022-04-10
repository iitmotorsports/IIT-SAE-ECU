/**
 * @file moduleOrder.hpp
 * @author IR
 * @brief
 * @version 0.1
 * @date 2022-04-07
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef __MODULEORDER_HPP__
#define __MODULEORDER_HPP__

#include "module.hpp"

#if CONF_LOGGING_ASCII_DEBUG
#include "core_pins.h"
#include "usb_serial.h"
#endif

#include <list>
#include <map>
#include <vector>

namespace Module {

static bitmapVal_t errC = 0;
static const Module_t *errorVec[maxModules] = {0};

struct Node {
    bool marked = false;
    const Module_t *module = nullptr;
    bitmapVal_t nID = 0U;
    bitmapVal_t dependencies = 0U;
    bitmapVal_t *depList[maxModules] = {0};
    Node(const Module_t *module, bitmapVal_t nID) : module{module}, nID{nID} {
    }
};

bool finder(Node *n, std::map<bitmapVal_t, Node *> &nodesR, std::list<Node *> &error, bool &done) {
    if (nodesR[n->nID]->marked) {
        error.push_back(n);
        return true;
    }
    nodesR[n->nID]->marked = true;

    for (bitmapVal_t *v : n->depList) {
        if (v == 0)
            break;
        auto node = nodesR.find(*v);
        if (node != nodesR.end()) {
            if (finder(node->second, nodesR, error, done)) {
                if (!done && error.front()->nID != n->nID)
                    error.push_back(n);
                else
                    done = true;
                return true;
            }
        }
    }
    return false;
}

bool Module_t::orderModules() {
    errC = 0;
    std::list<Node *> nodes;

    bitmapVal_t final = 0;
    bitmapVal_t mapped = 0;

    int c = 0;
    const Module_t *newModules[s_id];

    Log.d(ID, "Mapping module nodes");

    // Map modules to nodes
    for (size_t i = 0; i < s_id; i++) {
        const Module_t *mod = allModules[i];
        Node *node = new Node(mod, mod->id);

        bitmapVal_t dC = allModules[i]->count;

        if (dC == 0) { // mapped starts with 'root' nodes (no deps)
            Log.d(ID, "Root node", node->nID);
            node->marked = true;
            mapped |= node->nID;
            newModules[c++] = node->module;
            delete node;
        } else {
            Log.d(ID, "Dependent node", node->nID);
            const Module_t *const *dependents = allModules[i]->dependents;
            for (size_t j = 0; j < allModules[i]->count; j++) {
                const Module_t *dep = dependents[j];
                Log.d(ID, " ├─ Dependency", dep->id);
                node->dependencies |= dep->id;
                dep++;
            }
            Log.d(ID, " └ Final dependencies", node->dependencies);
            nodes.push_back(node);
        }
        final |= node->nID;
    }

    int last_c = c;

    Log.d(ID, "Tracing graph");

    while (mapped != final) {
        bitmapVal_t _mapped = mapped;

        typename std::list<Node *>::iterator iter;

        for (iter = nodes.begin(); iter != nodes.end();) {
            Node *node = *iter;
            if ((mapped & node->dependencies) && !(~mapped & node->dependencies)) {
                _mapped |= node->nID;
                newModules[c++] = node->module;
                delete node;
                iter = nodes.erase(iter);
            } else {
                iter++;
            }
        }

        if (last_c == c) { // Circular dependency

            Log.f(ID, "Circular dependency detected");

            std::list<Node *> error;

            std::map<bitmapVal_t, Node *> nodesR;
            for (Node *node : nodes) {
                nodesR[node->nID] = node;
            }

            bool done = false;

            for (Node *node : nodes) {
                if (finder(node, nodesR, error, done))
                    break;

                for (auto &n : nodesR) {
                    n.second->marked = false;
                }
            }

            for (Node *node : error) {
                errorVec[errC++] = node->module;
            }
#if CONF_LOGGING_ASCII_DEBUG
            for (bitmapVal_t i = 0; i < errC - 1; i++) {
                Serial.print(errorVec[i]->id);
                Serial.print(" <---> ");
            }
            Serial.println(errorVec[errC - 1]->id);
#else
            for (bitmapVal_t i = 0; i < errC - 1; i++) {
                Log.e(ID, "Depends on vv", errorVec[i]->id);
            }
            Log.e(ID, "Circles back to start ^^", errorVec[errC - 1]->id);
#endif
            return false;
        }

        last_c = c;

        mapped |= _mapped;
    }

    for (Node *node : nodes) {
        delete node;
    }

    Log.d(ID, "Modules ordered", c);

    std::copy(newModules, newModules + s_id, allModules);
    return true;
}

} // namespace Module

#endif // __MODULEORDER_HPP__