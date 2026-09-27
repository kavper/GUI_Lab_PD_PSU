#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenBatteryView::ScreenBatteryView()
    : divider(0)
{
}

void ScreenBatteryView::setupScreen()
{
    ScreenBatteryViewBase::setupScreen();
    refresh();
}

void ScreenBatteryView::tearDownScreen()
{
    ScreenBatteryViewBase::tearDownScreen();
}

void ScreenBatteryView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenBatteryView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_battery(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), BatteryBodyBuffer, BATTERYBODY_SIZE);
    BatteryBody.invalidate();
}


