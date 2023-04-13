#include "pump.h"

// DAC101C085

namespace Pump {
// TODO: ensure i2c can be restarted like this
void start() {
    static bool started = false;
    if (!started) {
        started = true;
        Wire1.setClock(100 * 1000); // Set the clock speed before calling begin()
        Wire1.begin();
    }
}

void set(uint8_t val) {
    Wire1.beginTransmission(PUMP_I2C_ADDR);
    Wire1.write(0b00001111);
    Wire1.write((val / 100) * 255);
    Wire1.endTransmission(true);
}

} // namespace Pump