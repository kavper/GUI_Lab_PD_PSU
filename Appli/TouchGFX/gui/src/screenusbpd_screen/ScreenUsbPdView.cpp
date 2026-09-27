#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenUsbPdView::ScreenUsbPdView()
    : divider(0)
{
}

void ScreenUsbPdView::setupScreen()
{
    ScreenUsbPdViewBase::setupScreen();
    refresh();
}

void ScreenUsbPdView::tearDownScreen()
{
    ScreenUsbPdViewBase::tearDownScreen();
}

void ScreenUsbPdView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenUsbPdView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_usb(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), UsbBodyBuffer, USBBODY_SIZE);
    UsbBody.invalidate();
}

void ScreenUsbPdView::usbAuto()
{
    psu_app_usb_role("AUTO", 1);
    refresh();
}
void ScreenUsbPdView::usbSink()
{
    psu_app_usb_role("SINK", 1);
    refresh();
}
void ScreenUsbPdView::usbSource()
{
    psu_app_usb_role("SOURCE", 1);
    refresh();
}

