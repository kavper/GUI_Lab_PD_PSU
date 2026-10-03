#ifndef LABTEXT_HPP
#define LABTEXT_HPP

#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>
#include <stdint.h>
#include <stdio.h>

inline void lab_put(touchgfx::Unicode::UnicodeChar* buf, uint16_t n, const char* ascii)
{
    if (ascii == 0)
        ascii = "";
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), buf, n);
}

inline touchgfx::colortype lab_cyan()
{
    return touchgfx::Color::getColorFromRGB(36, 87, 230);
}
inline touchgfx::colortype lab_text()
{
    return touchgfx::Color::getColorFromRGB(23, 35, 55);
}
inline touchgfx::colortype lab_muted()
{
    return touchgfx::Color::getColorFromRGB(96, 112, 133);
}
inline touchgfx::colortype lab_green()
{
    return touchgfx::Color::getColorFromRGB(22, 117, 72);
}
inline touchgfx::colortype lab_amber()
{
    return touchgfx::Color::getColorFromRGB(168, 91, 5);
}
inline touchgfx::colortype lab_red()
{
    return touchgfx::Color::getColorFromRGB(180, 62, 69);
}

template <typename Widget>
inline void lab_show(Widget& widget, touchgfx::Unicode::UnicodeChar* buf, uint16_t n,
                     const char* ascii, touchgfx::colortype color)
{
    lab_put(buf, n, ascii);
    widget.setColor(color);
    widget.invalidate();
}

inline void lab_ms(char* dst, size_t n, uint32_t ms)
{
    if (ms >= 1000U && (ms % 1000U) == 0U)
        (void)snprintf(dst, n, "%u s", (unsigned)(ms / 1000U));
    else if (ms >= 1000U)
        (void)snprintf(dst, n, "%u.%u s", (unsigned)(ms / 1000U), (unsigned)((ms % 1000U) / 100U));
    else
        (void)snprintf(dst, n, "%u ms", (unsigned)ms);
}

inline void lab_slew(char* dst, size_t n, uint32_t slew)
{
    if (slew == 0U)
        (void)snprintf(dst, n, "STEP");
    else
        (void)snprintf(dst, n, "%u mV/s", (unsigned)slew);
}

inline void lab_ip(char* dst, size_t n, const uint8_t ip[4])
{
    if (ip[0] == 0U && ip[1] == 0U && ip[2] == 0U && ip[3] == 0U)
        (void)snprintf(dst, n, "UNBOUND");
    else
        (void)snprintf(dst, n, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

inline void lab_mac(char* dst, size_t n, const uint8_t mac[6])
{
    int any = 0;
    for (int i = 0; i < 6; ++i)
        any |= mac[i];
    if (!any)
        (void)snprintf(dst, n, "UNASSIGNED");
    else
        (void)snprintf(dst, n, "%02X:%02X:%02X:%02X:%02X:%02X",
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

inline const char* lab_g4_link(uint8_t link)
{
    if (link == 1U)
        return "ONLINE";
    if (link == 2U)
        return "STALE";
    return "OFFLINE";
}

inline touchgfx::colortype lab_link_color(uint8_t link)
{
    if (link == 1U)
        return lab_green();
    if (link == 2U)
        return lab_amber();
    return lab_muted();
}

template <typename Button>
inline void lab_enable(Button& button, bool enabled)
{
    const uint8_t alpha = enabled ? 255 : 88;
    if (button.getAlpha() == alpha && button.isTouchable() == enabled) return;
    button.setTouchable(enabled);
    button.setAlpha(alpha);
    button.invalidate();
}

#endif
