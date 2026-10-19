#include <gui/screenbms_screen/ScreenBmsView.hpp>
#include <gui/common/LabText.hpp>
#include <string.h>
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

void ScreenBmsView::handleClickEvent(const touchgfx::ClickEvent& event)
{
    swipe.onClick(event, false);
    ScreenBmsViewBase::handleClickEvent(event);
}

void ScreenBmsView::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    if (swipe.onGesture(event))
    {
        application().gotoScreenSettingsScreenWipeTransitionEast();
        return;
    }
    ScreenBmsViewBase::handleGestureEvent(event);
}

void ScreenBmsView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

static touchgfx::colortype channel_color(const char *text)
{
    if (strcmp(text, "MISSING") == 0)
        return lab_muted();
    if (strcmp(text, "STALE") == 0)
        return lab_amber();
    return lab_cyan();
}

void ScreenBmsView::refresh()
{
    PsuBmsFrame frame;
    PsuBmsFace face;
    touchgfx::colortype stateColor;
    psu_bms_read(&frame);
    psu_bms_face(&frame, &face);
    lab_show(Cell1Value, Cell1ValueBuffer, CELL1VALUE_SIZE, face.channel[0], channel_color(face.channel[0]));
    lab_show(Cell2Value, Cell2ValueBuffer, CELL2VALUE_SIZE, face.channel[1], channel_color(face.channel[1]));
    lab_show(Cell3Value, Cell3ValueBuffer, CELL3VALUE_SIZE, face.channel[2], channel_color(face.channel[2]));
    lab_show(Cell4Value, Cell4ValueBuffer, CELL4VALUE_SIZE, face.channel[3], channel_color(face.channel[3]));
    if (strcmp(face.state, "OFFLINE") == 0)
        stateColor = lab_muted();
    else if (strcmp(face.state, "IDLE") == 0)
        stateColor = lab_text();
    else
        stateColor = lab_green();
    lab_show(BmsLink, BmsLinkBuffer, BMSLINK_SIZE, face.state, stateColor);
    lab_show(FetChg, FetChgBuffer, FETCHG_SIZE, face.fet_chg,
             strcmp(face.fet_chg, "ON") == 0 ? lab_green() : lab_muted());
    lab_show(FetDsg, FetDsgBuffer, FETDSG_SIZE, face.fet_dsg,
             strcmp(face.fet_dsg, "ON") == 0 ? lab_green() : lab_muted());
    lab_show(FetBal, FetBalBuffer, FETBAL_SIZE, face.balance,
             (strcmp(face.balance, "OFF") == 0 || strcmp(face.balance, "--") == 0) ? lab_muted() : lab_amber());
    lab_show(BmsFault, BmsFaultBuffer, BMSFAULT_SIZE, face.summary,
             (strcmp(face.fault, "OK") == 0) ? lab_green() :
             (strcmp(face.state, "OFFLINE") == 0) ? lab_amber() : lab_red());
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
