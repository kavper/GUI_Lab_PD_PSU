#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
#include <gui/common/LabText.hpp>
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

void ScreenBatteryView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenBatteryView::refresh()
{
    PsuSnapshot snap;
    char buf[24];
    uint32_t pack = 0;
    unsigned cells = 0;
    unsigned i;
    psu_app_ensure();
    psu_snapshot(&snap);
    if (!snap.bms_valid)
    {
        lab_show(BatPack, BatPackBuffer, BATPACK_SIZE, "--", lab_muted());
        lab_show(BatCells, BatCellsBuffer, BATCELLS_SIZE, "0", lab_muted());
        lab_show(BatTemp, BatTempBuffer, BATTEMP_SIZE, "--", lab_muted());
        lab_show(BatFault, BatFaultBuffer, BATFAULT_SIZE,
                 snap.bms_fault[0] ? snap.bms_fault : "NO PACK DATA", lab_amber());
        return;
    }
    for (i = 0; i < 4U; ++i)
    {
        if (snap.cell_mv[i] > 0U)
            ++cells;
        pack += snap.cell_mv[i];
    }
    psu_format_voltage(buf, sizeof(buf), pack);
    lab_show(BatPack, BatPackBuffer, BATPACK_SIZE, buf, lab_cyan());
    (void)snprintf(buf, sizeof(buf), "%u", cells);
    lab_show(BatCells, BatCellsBuffer, BATCELLS_SIZE, buf, lab_text());
    psu_format_centi_c(buf, sizeof(buf), snap.bms_temp_centi[0]);
    lab_show(BatTemp, BatTempBuffer, BATTEMP_SIZE, buf, lab_text());
    lab_show(BatFault, BatFaultBuffer, BATFAULT_SIZE,
             snap.bms_fault[0] ? snap.bms_fault : "OK",
             snap.bms_fault[0] ? lab_red() : lab_green());
}
