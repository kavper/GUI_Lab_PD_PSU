#ifndef DISPLAY_VOLTAGE_FILTER_HPP
#define DISPLAY_VOLTAGE_FILTER_HPP
#include <stdint.h>

// Presentation only: never feed this value back to control or protections.
class DisplayVoltageFilter
{
public:
    DisplayVoltageFilter() { reset(); }
    void reset() { sum = 0; count = 0; head = 0; shown = 0; lastSerial = 0; }
    uint32_t update(uint32_t mv, uint32_t sampleMs, uint32_t serial,
                    bool valid, bool bypass = false)
    {
        if (!valid) { reset(); return mv; }
        // A GUI refresh of the same METER must not count as another reading.
        if (count && serial == lastSerial) return shown;
        const uint32_t previous = count ? values[(head + count - 1U) % 4U] : mv;
        const uint32_t difference = mv > previous ? mv - previous : previous - mv;
        // DMM-style moving average with a step window. Never average across
        // a real step or a start/stop transition. No deadband or held voltage.
        if (bypass || difference >= 50U || mv == 0U) reset();
        // At most four received readings and at most 80 ms of history.
        while (count && sampleMs - times[head] > 80U) removeOldest();
        if (count == 4U) removeOldest();
        const unsigned slot = (head + count) % 4U;
        values[slot] = mv;
        times[slot] = sampleMs;
        sum += mv;
        ++count;
        lastSerial = serial;
        shown = static_cast<uint32_t>((sum + count / 2U) / count);
        return shown;
    }
private:
    void removeOldest()
    {
        sum -= values[head];
        head = (head + 1U) % 4U;
        --count;
    }
    uint64_t sum;
    uint32_t values[4], times[4], shown, lastSerial;
    unsigned count, head;
};
#endif
