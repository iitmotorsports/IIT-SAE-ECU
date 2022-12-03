#include <stdint.h>

#include "Canbus.h"
#include "ECUGlobalConfig.h"
#include "Log.h"

#include "SDBC_class.hpp"
#include "SDBC_gen.hpp"

namespace SDBC {

LOG_TAG ID = "SDBC";

template <typename T>
void post(const SIG signal, T data) {
    if (ACTIVE_NODE.nodeID == signal.nodeID)
        ACTIVE_NODE.setSig(signal, *((uint64_t *)&data));
#ifdef DEBUG
    else
        Log.w(ID, "Posting signal not relevant to node", signal.nodeID);
#endif
}

void *get(const SIG signal) {
    BACK_ECU.
}

} // namespace SDBC