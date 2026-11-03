#ifndef KITPAGE_HPP
#define KITPAGE_HPP
// Helpers shared by the kit-built pages (formatting and page setup).
#include <gui/common/Kit.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/TelemetryLayout.hpp>
#include <stdio.h>
#include <string.h>

namespace kit {
inline FrontendApplication* app() { return static_cast<FrontendApplication*>(touchgfx::Application::getInstance()); }

// Value in milli-units as "12.345 V" (decimals 1..3).
inline void milli(char* out, size_t n, int64_t v, const char* unit, int decimals = 3)
{
    const int64_t a = v < 0 ? -v : v;
    const int64_t div = decimals == 1 ? 100 : decimals == 2 ? 10 : 1;
    snprintf(out, n, decimals == 1 ? "%s%ld.%01ld%s%s" : decimals == 2 ? "%s%ld.%02ld%s%s" : "%s%ld.%03ld%s%s",
             v < 0 ? "-" : "", (long)(a / 1000), (long)(a % 1000 / div), unit[0] ? " " : "", unit);
}
// Deci-units (0.1 °C, 0.1 %) as "25.3 °C".
inline void deci(char* out, size_t n, int64_t v, const char* unit)
{
    const int64_t a = v < 0 ? -v : v;
    snprintf(out, n, "%s%ld.%ld %s", v < 0 ? "-" : "", (long)(a / 10), (long)(a % 10), unit);
}
}

// Hide the Designer page (kept for its generated callbacks) and claim the page.
#define KIT_PAGE_BEGIN(page) do { \
    kit::app()->setSwipePage(page); kit::clearNoSwipe(); \
    kit::hideAll(getRootContainer()); LabBackground.setVisible(true); } while (0)

#define KIT_PAGE_HEADER(page) do { header.setup(FrontendApplication::pageInfo(page)); add(header); } while (0)
#endif
