#include "Canbus.h"

#include "SDBC.def"
#include "WProgram.h"

namespace IO {

#define X(name, ID, c_type) inline void name(c_type val);

struct __WRITE {
    inline void ONBOARD_LED(bool val);

    inline void CHARGE_SIGNAL(bool val);

    inline void WHEEL_SPEED_BACK_LEFT(uint32_t data);
};

#undef X
#define X(name, ID, c_type) inline c_type name();

struct __READ {
    inline bool ONBOARD_LED();

    inline bool CHARGE_SIGNAL();

    inline uint32_t WHEEL_SPEED_BACK_LEFT();
};

static __WRITE WRITE;
static __READ READ;

#undef X

void reset();

} // namespace IO