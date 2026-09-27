#include <gui/screenprotection_screen/ScreenProtectionView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_format.h"
#include "psu_limits.h"
}

ScreenProtectionView::ScreenProtectionView()
    : divider(0)
{
}

void ScreenProtectionView::setupScreen()
{
    ScreenProtectionViewBase::setupScreen();
    refresh();
}

void ScreenProtectionView::tearDownScreen()
{
    ScreenProtectionViewBase::tearDownScreen();
}

void ScreenProtectionView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenProtectionView::refresh()
{
    PsuSnapshot snap;
    char buf[24];
    psu_app_ensure();
    psu_snapshot(&snap);
    if (snap.fault_latched)
    {
        lab_show(ProtState, ProtStateBuffer, PROTSTATE_SIZE, "FAULT", lab_red());
        lab_show(ProtNote, ProtNoteBuffer, PROTNOTE_SIZE, snap.fault[0] ? snap.fault : "LATCHED", lab_red());
    }
    else if (snap.output_confirmed)
    {
        lab_show(ProtState, ProtStateBuffer, PROTSTATE_SIZE, "OUTPUT ON", lab_red());
        lab_show(ProtNote, ProtNoteBuffer, PROTNOTE_SIZE, "LIVE", lab_red());
    }
    else
    {
        lab_show(ProtState, ProtStateBuffer, PROTSTATE_SIZE, "SAFE", lab_green());
        lab_show(ProtNote, ProtNoteBuffer, PROTNOTE_SIZE, "OUTPUT OFF", lab_muted());
    }
    psu_format_voltage(buf, sizeof(buf), PSU_VOLTAGE_MAX_MV);
    lab_show(ProtVlim, ProtVlimBuffer, PROTVLIM_SIZE, buf, lab_cyan());
    psu_format_current_ma(buf, sizeof(buf), PSU_CURRENT_MAX_MA);
    lab_show(ProtIlim, ProtIlimBuffer, PROTILIM_SIZE, buf, lab_amber());
}

void ScreenProtectionView::clearFault()
{
    psu_app_clear_fault();
    refresh();
}
