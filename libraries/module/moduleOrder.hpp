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

#include <algorithm>
#include <list>
#include <map>
#include <unordered_map>
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

const Module_t **orderModules(Module_t *modules[], size_t totalCount) {
    errC = 0;
    std::list<Node *> nodes;
    // std::map<const Module_t *, bitmapVal_t> moduleMap;

    bitmapVal_t final = 0;
    bitmapVal_t mapped = 0;

    int c = 0;
    const Module_t *newModules[totalCount];

    Log.d(ID, "mapping modules");

    // Map modules to nodes
    for (size_t i = 0; i < totalCount; i++) {
        const Module_t *mod = modules[i];
        Node *node = new Node(mod, mod->id);
        // moduleMap[mod] = mod->id;

        size_t j = 0;
        const Module_t **dependents = modules[i]->dependents;
        for (; j < modules[i]->count; j++) {
            const Module_t *dep = dependents[j];
            // dep->print();
            // node->depList[j] |= &dep->id;
            Log.d(ID, "dependent", dep->id);
            node->dependencies |= dep->id;
            dep++;
        }

        if (j == 0) { // mapped starts with 'core' nodes (no deps)
            Log.d(ID, "core node", node->nID);
            node->marked = true;
            mapped |= node->nID;
            newModules[c++] = node->module;
            delete node;
        } else {
            Log.d(ID, "final dependents", node->dependencies);
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

            Log.f(ID, "Circular Dependency");

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

            return errorVec;
        }

        last_c = c;

        mapped |= _mapped;
    }

    for (Node *node : nodes) {
        delete node;
    }

    Log.d(ID, "module count", c);

    std::copy(newModules, newModules + totalCount, modules);
    return errorVec;
}

} // namespace Module

#endif // __MODULEORDER_HPP__