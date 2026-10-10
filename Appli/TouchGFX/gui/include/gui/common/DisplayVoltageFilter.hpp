#ifndef DISPLAY_VOLTAGE_FILTER_HPP
#define DISPLAY_VOLTAGE_FILTER_HPP
#include <stdint.h>

// Presentation only: never feed this value back to control or protections.
class DisplayVoltageFilter
{
public:
    DisplayVoltageFilter() : value(0), lastMs(0), ready(false), fast(false) {}
    void reset() { ready = false; fast = false; }
    uint32_t update(uint32_t mv, uint32_t now, bool valid)
    {
        if (!valid) { reset(); return mv; }
        const uint32_t dt = now - lastMs;
        lastMs = now;
        const double delta = static_cast<double>(mv) - value;
        const double magnitude = delta < 0 ? -delta : delta;
        // Initialise immediately after screen entry, stale data or a long pause.
        if (!ready || dt > 500U) { value = mv; ready = true; fast = false; }
        else if (dt != 0U)
        {
            // 250 ms for small jitter; 35 ms for real steps >= 100 mV.
            if (magnitude >= 100.0) fast = true;
            else if (magnitude < 5.0) fast = false;
            const double tau = fast ? 35.0 : 250.0;
            value += delta * static_cast<double>(dt) / (tau + dt);
        }
        return static_cast<uint32_t>(value + 0.5);
    }
private:
    double value;
    uint32_t lastMs;
    bool ready;
    bool fast;
};
#endif
