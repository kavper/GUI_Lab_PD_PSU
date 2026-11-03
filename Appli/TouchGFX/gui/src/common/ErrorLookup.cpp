#include <gui/common/ErrorCatalog.hpp>
#include <gui/common/TelemetryData.hpp>
#include <stdio.h>
#include <string.h>

namespace err {
const Entry* find(GroupId g, uint32_t value)
{
    const Group& group = groups[g];
    for (unsigned i = 0; i < group.count; ++i) if (group.entries[i].value == value) return &group.entries[i];
    return 0;
}

unsigned active(GroupId g, uint32_t mask, const Entry** out, unsigned max, uint32_t* unknown, uint8_t minSeverity)
{
    const Group& group = groups[g];
    uint32_t known = 0;
    unsigned n = 0;
    for (int sev = ES_FAULT; sev >= (int)minSeverity; --sev)
        for (unsigned i = 0; i < group.count; ++i) {
            const Entry& e = group.entries[i];
            if (sev == ES_FAULT) known |= e.value;
            if (e.severity == sev && (mask & e.value) == e.value && e.value && n < max) out[n++] = &e;
        }
    if (unknown) *unknown = mask & ~known;
    return n;
}

void symbols(GroupId g, uint32_t mask, char* out, size_t n, uint8_t minSeverity)
{
    const Entry* list[8];
    uint32_t unknown = 0;
    const unsigned count = active(g, mask, list, 8, &unknown, minSeverity);
    // Most severe symbol, then how many more are set: fits small boxes.
    const unsigned more = (count ? count - 1 : 0) + (unknown ? 1 : 0);
    if (count) snprintf(out, n, more ? "%s +%u" : "%s", list[0]->symbol, more);
    else if (unknown) snprintf(out, n, "0x%lX", (unsigned long)unknown);
    else out[0] = 0;
}

const char* latchText(const char* reason, char* out, size_t n)
{
    unsigned long mask = 0;
    if (!reason) return "";
    if (sscanf(reason, "G4 FAULT 0x%lx", &mask) == 1 && mask) {
        const Entry* list[8];
        const unsigned count = active(E_PSU, (uint32_t)mask, list, 8);
        if (count) { snprintf(out, n, count > 1 ? "%s +%u more" : "%s", list[0]->text, count - 1); return out; }
    }
    static const char* const known[][2] = {
        { "G4: G0 MEAS_LOST", "G0 measurements lost" }, { "G4: G0 POWER_KILL", "G0 kill signal active" },
        { "G4 CONTROL FAULT", "G4 control fault" }, { "G0 OFFLINE", "G0 offline" } };
    for (unsigned i = 0; i < sizeof(known) / sizeof(known[0]); ++i) if (!strcmp(reason, known[i][0])) return known[i][1];
    return reason;
}

namespace {
// Collect the issues of one group into a ranked list.
struct Issues {
    const char* text[12]; uint8_t severity[12]; unsigned count; uint32_t unknown;
    Issues() : count(0), unknown(0) {}
    void add(GroupId g, int64_t value, bool present)
    {
        if (!present || !value) return;
        const Entry* list[8];
        uint32_t extra = 0;
        const unsigned n = active(g, (uint32_t)value, list, 8, &extra, ES_WARNING);
        for (unsigned i = 0; i < n && count < 12; ++i) { text[count] = list[i]->text; severity[count] = list[i]->severity; ++count; }
        unknown |= extra;
    }
    void addEnum(GroupId g, int64_t value, bool present)
    {
        if (!present) return;
        const Entry* e = find(g, (uint32_t)value);
        if (e && e->severity >= ES_WARNING && count < 12) { text[count] = e->text; severity[count] = e->severity; ++count; }
    }
    int write(char* out, size_t n) const
    {
        out[0] = 0;
        if (!count && !unknown) return -1;
        unsigned best = 0;
        for (unsigned i = 1; i < count; ++i) if (severity[i] > severity[best]) best = i;
        if (!count) { snprintf(out, n, "Unknown status bits 0x%lX", (unsigned long)unknown); return ES_WARNING; }
        if (count > 1) snprintf(out, n, "%s +%u more", text[best], count - 1);
        else snprintf(out, n, "%s", text[best]);
        return severity[best];
    }
};
}

int bmsIssue(const TelemetryRecord& tb, char* out, size_t n)
{
    int64_t fault = 0, sa = 0, sb = 0, sc = 0, alarm = 0;
    const bool hf = tb.get("fault", fault);
    Issues issues;
    // The summary bits SAFETY / ALERT are explained by the detail registers.
    const bool hsa = tb.get("sa", sa), hsb = tb.get("sb", sb), hsc = tb.get("sc", sc);
    const bool detailed = (hsa && sa) || (hsb && sb) || (hsc && sc);
    issues.add(E_BMS_SA, sa, hsa);
    issues.add(E_BMS_SB, sb, hsb);
    issues.add(E_BMS_SC, sc, hsc);
    issues.add(E_BMS_FAULT, detailed ? (fault & ~0x60) : (fault & ~0x40), hf);
    const bool ha = tb.get("alarm", alarm);
    issues.add(E_BMS_ALARM, alarm & 0x2000, ha); // only Permanent Fail needs attention here
    return issues.write(out, n);
}

int outputIssue(const TelemetryRecord& meter, const TelemetryRecord& aux, char* out, size_t n)
{
    int64_t psu = 0, g0 = 0, stop = 0;
    Issues issues;
    const bool hp = meter.get("fault", psu), hg = meter.get("g0_fault", g0), hs = aux.get("stop_reason", stop);
    issues.add(E_PSU, psu, hp);
    issues.add(E_G0, g0, hg);
    issues.addEnum(E_STOP, stop, hs && stop);
    return issues.write(out, n);
}
}
