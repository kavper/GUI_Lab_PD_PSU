#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
#include <gui/common/LabText.hpp>
#include <string.h>
extern "C" {
#include "psu_app.h"
#include "psu_format.h"
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

void ScreenBatteryView::handleClickEvent(const touchgfx::ClickEvent& event)
{
    swipe.onClick(event, false);
    ScreenBatteryViewBase::handleClickEvent(event);
}

void ScreenBatteryView::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    if (swipe.onGesture(event))
    {
        application().gotoScreenSettingsScreenWipeTransitionEast();
        return;
    }
    ScreenBatteryViewBase::handleGestureEvent(event);
}

void ScreenBatteryView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

static const char *readout_or_dash(const char *text)
{
    if (text == 0 || text[0] == '\0' || strcmp(text, "UNAVAILABLE") == 0 || strcmp(text, "--") == 0)
        return "--";
    if ((text[0] < '0' || text[0] > '9') && text[0] != '-')
        return "--";
    return text;
}

void ScreenBatteryView::refresh()
{
    PsuBmsFrame frame;
    PsuBmsFace face;
    char note[80];
    char temp[16];
    psu_bms_read(&frame);
    psu_bms_face(&frame, &face);
    lab_show(BatPack, BatPackBuffer, BATPACK_SIZE, readout_or_dash(face.pack_v),
             frame.valid ? lab_cyan() : lab_muted());
    lab_show(BatCells, BatCellsBuffer, BATCELLS_SIZE, face.channels_line, lab_text());
    if (!frame.valid)
        lab_show(BatTemp, BatTempBuffer, BATTEMP_SIZE, "--", lab_muted());
    else
    {
        psu_format_centi_c(temp, sizeof(temp), frame.temp_centi[0]);
        lab_show(BatTemp, BatTempBuffer, BATTEMP_SIZE, temp, lab_text());
    }
    (void)snprintf(note, sizeof(note), "%s  %s  %s", face.state, face.soc, face.fault);
    lab_show(BatFault, BatFaultBuffer, BATFAULT_SIZE, note,
             strcmp(face.fault, "OK") == 0 ? lab_green() :
             strcmp(face.state, "OFFLINE") == 0 ? lab_amber() : lab_red());
}
