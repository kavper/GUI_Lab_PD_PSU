#include <gui/screenservice_screen/ScreenServiceView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenServiceView::ScreenServiceView()
    : divider(0)
{
}

void ScreenServiceView::setupScreen()
{
    ScreenServiceViewBase::setupScreen();
    refresh();
}

void ScreenServiceView::tearDownScreen()
{
    ScreenServiceViewBase::tearDownScreen();
}

void ScreenServiceView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenServiceView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_service(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), ServiceBodyBuffer, SERVICEBODY_SIZE);
    ServiceBody.invalidate();
}

void ScreenServiceView::serviceToggle()
{
    psu_app_service(!psu_g4()->service_mode);
    refresh();
}
void ScreenServiceView::serviceHelp()
{
    psu_app_service_cmd("HELP");
    refresh();
}
void ScreenServiceView::serviceStatus()
{
    psu_app_service_cmd("STATUS");
    refresh();
}

