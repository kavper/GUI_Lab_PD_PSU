#include <gui/screenbms_screen/ScreenBmsView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenBmsView::ScreenBmsView()
    : divider(0)
{
}

void ScreenBmsView::setupScreen()
{
    ScreenBmsViewBase::setupScreen();
    refresh();
}

void ScreenBmsView::tearDownScreen()
{
    ScreenBmsViewBase::tearDownScreen();
}

void ScreenBmsView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenBmsView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_bms(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), BmsBodyBuffer, BMSBODY_SIZE);
    BmsBody.invalidate();
}

void ScreenBmsView::bmsConfigure()
{
    psu_app_bms_cmd("BMS");
    refresh();
}
void ScreenBmsView::bmsOff()
{
    psu_app_bms_cmd("BMS OFF");
    refresh();
}
void ScreenBmsView::bmsClear()
{
    psu_app_bms_cmd("CLR");
    refresh();
}
void ScreenBmsView::bmsRefresh()
{
    psu_app_bms_cmd("STATUS");
    refresh();
}

