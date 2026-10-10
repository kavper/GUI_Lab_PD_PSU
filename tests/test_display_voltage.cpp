#include <gui/common/DisplayVoltageFilter.hpp>
#include <assert.h>
#include <stdio.h>

int main()
{
    DisplayVoltageFilter f;
    assert(f.update(5000, 0, true) == 5000);
    // Alternating 10 mV jitter stays near the centre after settling.
    uint32_t v = 0;
    for (uint32_t t = 16; t <= 1600; t += 16)
    {
        v = f.update((t / 16) % 2 ? 5010 : 4990, t, true);
        if (t > 500) assert(v >= 4998 && v <= 5002);
    }
    // A 1 V step reaches within 1% in 200 ms.
    for (uint32_t t = 1616; t <= 1808; t += 16) v = f.update(6000, t, true);
    assert(v >= 5990 && v <= 6000);
    // A real collapse is shown promptly too.
    for (uint32_t t = 1824; t <= 2032; t += 16) v = f.update(0, t, true);
    assert(v < 100);
    f.update(0, 2048, false);
    assert(f.update(7000, 2064, true) == 7000);
    assert(f.update(9000, 3000, true) == 9000);
    assert(f.update(0, 3000, true) == 9000);
    f.reset();
    assert(f.update(1234, 3016, true) == 1234);
    puts("Display voltage filter tests passed");
}
