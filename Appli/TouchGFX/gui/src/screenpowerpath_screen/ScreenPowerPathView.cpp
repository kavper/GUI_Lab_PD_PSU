#include <gui/screenpowerpath_screen/ScreenPowerPathView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenPowerPathView::ScreenPowerPathView()
    : divider(0)
{
}

void ScreenPowerPathView::setupScreen()
{
    ScreenPowerPathViewBase::setupScreen();
    refresh();
}

void ScreenPowerPathView::tearDownScreen()
{
    ScreenPowerPathViewBase::tearDownScreen();
}

void ScreenPowerPathView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenPowerPathView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_power_path(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), PathBodyBuffer, PATHBODY_SIZE);
    PathBody.invalidate();
}

void ScreenPowerPathView::pathPermit()
{
    g4_permit(psu_g4(), 1, 0);
    refresh();
}
void ScreenPowerPathView::pathRemote()
{
    g4_remote(psu_g4(), 1, 0);
    refresh();
}

