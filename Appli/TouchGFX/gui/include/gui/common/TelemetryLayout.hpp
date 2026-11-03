#ifndef TELEMETRYLAYOUT_HPP
#define TELEMETRYLAYOUT_HPP
// Shared layout for the telemetry pages generated from the same Designer
// template (metric cards, left/right detail cards, action buttons). Used from
// inside each view's layout() because the widgets are protected members.
#include <gui/common/Kit.hpp>
#include <string.h>

namespace kit {
// Colour a live value by its unit or meaning, so V/A/W and states read the same on every page.
inline ui::Role valueRole(const char* text, bool fresh)
{
    if (!fresh || !text || !strcmp(text, "--")) return ui::MUTED;
    const size_t n = strlen(text);
    if (strstr(text, "FAULT") || strstr(text, "BLOCKED") || strstr(text, "ACTIVE")) return ui::NEGATIVE;
    if (strstr(text, "CLEAR") || strstr(text, "ALLOWED") || strstr(text, "RUNNING") || !strcmp(text, "ON")) return ui::POSITIVE;
    if (n >= 2 && text[n - 1] == 'V' && text[n - 2] == ' ') return ui::VOLT;
    if (n >= 2 && text[n - 1] == 'A' && text[n - 2] == ' ') return ui::AMP;
    if (n >= 2 && text[n - 1] == 'W' && text[n - 2] == ' ') return ui::WATT;
    return ui::TEXT;
}
}

#define KIT_HIDE_DESIGNER_HEADER() \
    kit::hide(LabHeader); kit::hide(ThemeHeaderDivider); kit::hide(PageTitle); kit::hide(PageFeedback); \
    kit::hide(AllOffButton); kit::hide(BackButton)

// Metric cards in one row (y 58, height 78).
#define KIT_METRIC(i, n) do { \
    const int w_ = (776 - ((n) - 1) * 10) / (n), x_ = 12 + (i) * (w_ + 10); \
    kit::place(MetricCard##i, x_, 58, w_, 78); \
    kit::place(MetricLabel##i, x_ + 12, 64, w_ - 20, 22); \
    kit::place(Metric##i, x_ + 12, 90, w_ - 16, 36); kit::retype(Metric##i, kit::M20); } while (0)

// Left / right detail cards (y 146 .. 402) with monospace key/value text.
#define KIT_DETAILS(top, height) do { \
    kit::place(LeftCard, 12, top, 383, height); kit::place(LeftHeading, 26, (top) + 8, 355, 24); \
    kit::place(LeftDetails, 26, (top) + 36, 360, (height) - 44); kit::retype(LeftDetails, kit::M14); \
    kit::place(RightCard, 405, top, 383, height); kit::place(RightHeading, 419, (top) + 8, 355, 24); \
    kit::place(RightDetails, 419, (top) + 36, 360, (height) - 44); kit::retype(RightDetails, kit::M14); } while (0)

#define KIT_ACTION(button, slot) kit::place(button, 12 + (slot) * 196, 414, 186, 54)

#define KIT_SHOW_METRIC(i, data) \
    lab_show(Metric##i, Metric##i##Buffer, METRIC##i##_SIZE, data.metric[i], ui::Theme::color(kit::valueRole(data.metric[i], data.fresh)))

#endif
