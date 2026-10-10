#include <gui/common/DisplayVoltageFilter.hpp>
#include <assert.h>
#include <stdio.h>

int main()
{
    DisplayVoltageFilter f;
    // Different first noisy readings must converge to the same real mean.
    for (unsigned phase = 0; phase < 2; ++phase)
    {
        f.reset();
        for (uint32_t i = 0; i < 20; ++i)
        {
            const uint32_t raw = (i + phase) % 2 ? 5010 : 4990;
            const uint32_t shown = f.update(raw, i * 20, i, true);
            if (i >= 3) assert(shown == 5000);
        }
    }
    // Even 1/5 mV DC changes converge exactly; no deadband hides the new level.
    for (uint32_t delta = 1; delta <= 5; delta += 4)
    {
        f.reset();
        for (uint32_t i = 0; i < 4; ++i) f.update(5000, i * 20, i, true);
        for (uint32_t i = 4; i < 8; ++i) f.update(5000 + delta, i * 20, i, true);
        assert(f.update(5000 + delta, 160, 8, true) == 5000 + delta);
    }

    // Redrawing one METER cannot repeatedly weight that reading.
    f.reset();
    assert(f.update(4990, 0, 1, true) == 4990);
    assert(f.update(5010, 20, 2, true) == 5000);
    for (unsigned i = 0; i < 100; ++i)
        assert(f.update(5010, 20, 2, true) == 5000);

    // Exact steps, collapse and distinct frames sharing a timestamp.
    assert(f.update(12000, 20, 3, true) == 12000);
    assert(f.update(5000, 20, 4, true) == 5000);
    assert(f.update(0, 40, 5, true) == 0);
    assert(f.update(12000, 60, 6, true) == 12000);

    // Actual start/stop bypasses averaging, including tiny changes.
    for (uint32_t i = 7; i < 20; ++i)
        assert(f.update(12000 + i, i * 20, i, true, true) == 12000 + i);

    // A slow ramp has a fixed short average lag, not holds or exponential rise.
    f.reset();
    for (uint32_t i = 0; i < 20; ++i)
    {
        const uint32_t v = f.update(5000 + i * 10, i * 20, i, true);
        if (i >= 3) assert(v == 5000 + i * 10 - 15);
    }
    // Fast ramps pass immediately in both directions.
    f.reset();
    for (uint32_t i = 0; i <= 270; ++i)
        assert(f.update(i * 100, i * 20, i, true) == i * 100);
    for (uint32_t i = 271; i <= 540; ++i)
        assert(f.update((540 - i) * 100, i * 20, i, true) == (540 - i) * 100);

    // Slow packets expire history using acquisition timestamps.
    f.reset();
    f.update(5000, 0, 1, true);
    assert(f.update(5020, 100, 2, true) == 5020);
    assert(f.update(5030, 200, 3, true) == 5030);
    f.update(0, 220, 4, false);
    assert(f.update(5010, 240, 5, true) == 5010);

    // Clock/serial wrapping and wide integers remain safe.
    f.reset();
    f.update(5000, UINT32_MAX - 10, UINT32_MAX, true);
    assert(f.update(5020, 9, 0, true) == 5010);
    f.reset();
    f.update(UINT32_MAX, 0, 1, true);
    assert(f.update(UINT32_MAX - 2, 20, 2, true) == UINT32_MAX - 1);
    puts("Display voltage moving-average tests passed");
}
