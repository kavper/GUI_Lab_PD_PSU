#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/common/LabText.hpp>
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
    PsuSnapshot snap;
    const char* role;
    psu_app_ensure();
    psu_snapshot(&snap);
    role = (snap.usb_role == 1U) ? "SINK" : (snap.usb_role == 2U) ? "SOURCE" : "AUTO";
    lab_show(RoleValue, RoleValueBuffer, ROLEVALUE_SIZE, role, lab_cyan());
    lab_show(PpsValue, PpsValueBuffer, PPSVALUE_SIZE, snap.pps_allowed ? "OPEN" : "LOCKED",
             snap.pps_allowed ? lab_green() : lab_amber());
    lab_show(UsbLink, UsbLinkBuffer, USBLINK_SIZE, lab_g4_link(snap.g4_link), lab_link_color(snap.g4_link));
    lab_show(UsbLast, UsbLastBuffer, USBLAST_SIZE, snap.g4_last_tx[0] ? snap.g4_last_tx : "--", lab_text());
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
