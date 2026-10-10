#include <gui/common/DisplayVoltageFilter.hpp>
#include <assert.h>
#include <stdio.h>

int main()
{
    DisplayVoltageFilter f;
    assert(f.update(5000, 0, true) == 5000);
    // Alternating 10 mV jitter is held without a temporal low-pass filter.
    uint32_t v = 0;
    for (uint32_t t = 16; t <= 1600; t += 16)
    {
        v = f.update((t / 16) % 2 ? 5010 : 4990, t, true);
        assert(v == 5000);
    }
    // Both step directions pass exactly on their first update, even at the
    // same timestamp: GUI tick timing cannot delay a new sample.
    assert(f.update(12000, 1600, true) == 12000);
    assert(f.update(5000, 1600, true) == 5000);
    assert(f.update(0, 1616, true) == 0);
    assert(f.update(5, 1632, true) == 0);
    assert(f.update(0, 1648, true) == 0);
    // Slow and fast ramps, rising and falling. Error is bounded in voltage,
    // never by a settling time. Changes > 10 mV must pass exactly.
    const uint32_t increments[] = {1, 5, 10, 11, 100, 1000};
    uint32_t now = 1664;
    for (unsigned i = 0; i < sizeof(increments)/sizeof(increments[0]); ++i)
    {
        f.reset();
        f.update(0, now, true);
        for (uint32_t raw = 0; raw <= 27000; raw += increments[i])
        {
            now += 16;
            v = f.update(raw, now, true);
            assert(v <= raw && raw - v <= 10);
            if (increments[i] > 10) assert(v == raw);
        }
        f.reset();
        f.update(27000, now, true);
        for (int32_t raw = 27000; raw >= 0; raw -= increments[i])
        {
            now += 16;
            v = f.update(static_cast<uint32_t>(raw), now, true);
            assert(v >= static_cast<uint32_t>(raw) && v - raw <= 10);
            if (increments[i] > 10) assert(v == static_cast<uint32_t>(raw));
        }
    }
    f.update(0, 2048, false);
    assert(f.update(7000, 2064, true) == 7000);
    assert(f.update(9000, 3000, true) == 9000);
    assert(f.update(0, 3000, true) == 0);
    f.reset();
    assert(f.update(1234, 3016, true) == 1234);
    puts("Display voltage filter tests passed");
}
