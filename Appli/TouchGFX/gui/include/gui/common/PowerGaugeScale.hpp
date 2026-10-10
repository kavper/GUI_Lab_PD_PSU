#ifndef POWER_GAUGE_SCALE_HPP
#define POWER_GAUGE_SCALE_HPP
#include <stdint.h>

// A symmetric display range, independent of electrical power limits.
class PowerGaugeScale
{
public:
    PowerGaugeScale() : rangeMw(100000), candidateMw(0), candidateSince(0) {}
    uint32_t range() const { return rangeMw; }
    void update(int64_t mw, uint32_t now, bool valid)
    {
        if (!valid) { candidateMw = 0; return; }
        const uint64_t magnitude = mw < 0 ? static_cast<uint64_t>(-(mw + 1)) + 1U : static_cast<uint64_t>(mw);
        // Leave 15% headroom and use readable 1/2/3/5 ranges.
        const uint64_t required = magnitude > 20000000U ? 20000000U : (magnitude * 23U + 19U) / 20U;
        const uint32_t wanted = niceRange(required);
        if (wanted > rangeMw) { rangeMw = wanted; candidateMw = 0; }
        else if (wanted == rangeMw) candidateMw = 0;
        else if (candidateMw != wanted) { candidateMw = wanted; candidateSince = now; }
        else if (now - candidateSince >= 5000U) { rangeMw = wanted; candidateMw = 0; }
    }
    int16_t position(int64_t mw) const
    {
        const int64_t bounded = mw < -static_cast<int64_t>(rangeMw) ? -static_cast<int64_t>(rangeMw)
                              : mw > rangeMw ? rangeMw : mw;
        return static_cast<int16_t>(bounded * 344 / rangeMw);
    }
private:
    static uint32_t niceRange(uint64_t required)
    {
        const uint32_t ranges[] = {100000,200000,300000,500000,1000000,
                                  2000000,3000000,5000000,10000000,20000000};
        for (unsigned i = 0; i < sizeof(ranges)/sizeof(ranges[0]); ++i)
            if (required <= ranges[i]) return ranges[i];
        return 20000000;
    }
    uint32_t rangeMw, candidateMw, candidateSince;
};
#endif
