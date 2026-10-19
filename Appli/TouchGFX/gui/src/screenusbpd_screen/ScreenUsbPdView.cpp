#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/common/LabText.hpp>
#include <string.h>
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

void ScreenUsbPdView::handleClickEvent(const touchgfx::ClickEvent& event)
{
    swipe.onClick(event, false);
    ScreenUsbPdViewBase::handleClickEvent(event);
}

void ScreenUsbPdView::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    if (swipe.onGesture(event))
    {
        application().gotoScreenSettingsScreenWipeTransitionEast();
        return;
    }
    ScreenUsbPdViewBase::handleGestureEvent(event);
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
    PsuUsbFace face;
    touchgfx::colortype linkColor = lab_muted();
    psu_usb_face(&face);
    lab_show(RoleValue, RoleValueBuffer, ROLEVALUE_SIZE, face.requested, lab_cyan());
    lab_show(ConfValue, ConfValueBuffer, CONFVALUE_SIZE, face.confirmed, lab_muted());
    lab_show(PpsValue, PpsValueBuffer, PPSVALUE_SIZE, face.pps,
             strcmp(face.pps, "OPEN") == 0 ? lab_green() : lab_amber());
    if (strcmp(face.link, "ONLINE") == 0)
        linkColor = lab_green();
    else if (strcmp(face.link, "STALE") == 0)
        linkColor = lab_amber();
    lab_show(UsbLink, UsbLinkBuffer, USBLINK_SIZE, face.link, linkColor);
    lab_show(FlowText, FlowTextBuffer, FLOWTEXT_SIZE, face.flow,
             strcmp(face.flow, "GAP") == 0 ? lab_amber() : lab_text());
    lab_show(UsbLast, UsbLastBuffer, USBLAST_SIZE, face.pdo,
             face.selected_row >= 0 ? lab_text() : lab_muted());
    lab_show(ContractText, ContractTextBuffer, CONTRACTTEXT_SIZE, face.contract, lab_text());
    FlowFill.invalidate();
    FlowFill.setX(face.fill_x);
    FlowFill.setWidth(face.fill_w > 0 ? face.fill_w : 0);
    FlowFill.invalidate();
    if (face.selected_row >= 0)
    {
        PdoMark.setPosition(UsbLast.getX(),
                            static_cast<int16_t>(UsbLast.getY() + face.selected_row * 16),
                            UsbLast.getWidth(), 16);
        PdoMark.setVisible(true);
    }
    else
    {
        PdoMark.setWidth(0);
        PdoMark.setVisible(false);
    }
    PdoMark.invalidate();
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
