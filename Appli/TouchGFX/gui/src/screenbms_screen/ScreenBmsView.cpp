#include <gui/screenbms_screen/ScreenBmsView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_format.h"
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
    PsuSnapshot snap;
    char buf[24];
    psu_app_ensure();
    psu_snapshot(&snap);

    if (!snap.bms_valid)
    {
        lab_show(Cell1Value, Cell1ValueBuffer, CELL1VALUE_SIZE, "--", lab_muted());
        lab_show(Cell2Value, Cell2ValueBuffer, CELL2VALUE_SIZE, "--", lab_muted());
        lab_show(Cell3Value, Cell3ValueBuffer, CELL3VALUE_SIZE, "--", lab_muted());
        lab_show(Cell4Value, Cell4ValueBuffer, CELL4VALUE_SIZE, "--", lab_muted());
        lab_show(BmsLink, BmsLinkBuffer, BMSLINK_SIZE, "OFFLINE", lab_muted());
        lab_show(FetChg, FetChgBuffer, FETCHG_SIZE, "OFF", lab_muted());
        lab_show(FetDsg, FetDsgBuffer, FETDSG_SIZE, "OFF", lab_muted());
        lab_show(FetBal, FetBalBuffer, FETBAL_SIZE, "OFF", lab_muted());
        lab_show(BmsFault, BmsFaultBuffer, BMSFAULT_SIZE,
                 snap.bms_fault[0] ? snap.bms_fault : "NO PACK DATA", lab_amber());
        return;
    }

    (void)snprintf(buf, sizeof(buf), "%u.%03u V", snap.cell_mv[0] / 1000U, snap.cell_mv[0] % 1000U);
    lab_show(Cell1Value, Cell1ValueBuffer, CELL1VALUE_SIZE, buf, lab_cyan());
    (void)snprintf(buf, sizeof(buf), "%u.%03u V", snap.cell_mv[1] / 1000U, snap.cell_mv[1] % 1000U);
    lab_show(Cell2Value, Cell2ValueBuffer, CELL2VALUE_SIZE, buf, lab_cyan());
    (void)snprintf(buf, sizeof(buf), "%u.%03u V", snap.cell_mv[2] / 1000U, snap.cell_mv[2] % 1000U);
    lab_show(Cell3Value, Cell3ValueBuffer, CELL3VALUE_SIZE, buf, lab_cyan());
    (void)snprintf(buf, sizeof(buf), "%u.%03u V", snap.cell_mv[3] / 1000U, snap.cell_mv[3] % 1000U);
    lab_show(Cell4Value, Cell4ValueBuffer, CELL4VALUE_SIZE, buf, lab_cyan());
    lab_show(BmsLink, BmsLinkBuffer, BMSLINK_SIZE, "ONLINE", lab_green());
    lab_show(FetChg, FetChgBuffer, FETCHG_SIZE, snap.bms_fet_chg ? "ON" : "OFF",
             snap.bms_fet_chg ? lab_green() : lab_muted());
    lab_show(FetDsg, FetDsgBuffer, FETDSG_SIZE, snap.bms_fet_dsg ? "ON" : "OFF",
             snap.bms_fet_dsg ? lab_green() : lab_muted());
    lab_show(FetBal, FetBalBuffer, FETBAL_SIZE, snap.bms_balancing ? "ON" : "OFF",
             snap.bms_balancing ? lab_amber() : lab_muted());
    lab_show(BmsFault, BmsFaultBuffer, BMSFAULT_SIZE,
             snap.bms_fault[0] ? snap.bms_fault : "OK",
             snap.bms_fault[0] ? lab_red() : lab_green());
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
