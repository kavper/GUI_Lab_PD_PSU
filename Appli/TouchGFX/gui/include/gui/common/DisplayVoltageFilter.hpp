#ifndef DISPLAY_VOLTAGE_FILTER_HPP
#define DISPLAY_VOLTAGE_FILTER_HPP
#include <stdint.h>

// Presentation only: never feed this value back to control or protections.
class DisplayVoltageFilter
{
public:
    DisplayVoltageFilter() : value(0), lastMs(0), ready(false) {}
    void reset() { ready = false; }
    uint32_t update(uint32_t mv, uint32_t now, bool valid)
    {
        if (!valid) { reset(); return mv; }
        const uint32_t dt = now - lastMs;
        lastMs = now;
        const uint32_t difference = mv > value ? mv - value : value - mv;
        // No averaging or time delay: only hold insignificant last-digit jitter.
        // Every change outside this band passes unchanged in this update.
        // Even a slow ramp can never get more than 10 mV behind its raw sample.
        if (!ready || dt > 500U || difference > 10U || mv == 0U)
            value = mv;
        ready = true;
        return value;
    }
private:
    uint32_t value;
    uint32_t lastMs;
    bool ready;
};
#endif
