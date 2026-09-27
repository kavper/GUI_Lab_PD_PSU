#include <gui/screennetwork_screen/ScreenNetworkView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenNetworkView::ScreenNetworkView()
    : divider(0)
{
}

void ScreenNetworkView::setupScreen()
{
    ScreenNetworkViewBase::setupScreen();
    refresh();
}

void ScreenNetworkView::tearDownScreen()
{
    ScreenNetworkViewBase::tearDownScreen();
}

void ScreenNetworkView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenNetworkView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_network(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), NetBodyBuffer, NETBODY_SIZE);
    NetBody.invalidate();
}


