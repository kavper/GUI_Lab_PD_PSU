#include <gui/screenextcharger_screen/ScreenExtChargerView.hpp>
#include <gui/common/LabText.hpp>
#include <string.h>
extern "C" {
#include "psu_app.h"
#include "psu_charger.h"
#include "psu_format.h"
}

ScreenExtChargerView::ScreenExtChargerView()
    : divider(0)
{
}

void ScreenExtChargerView::setupScreen()
{
    ScreenExtChargerViewBase::setupScreen();
    refresh();
}

void ScreenExtChargerView::tearDownScreen()
{
    ScreenExtChargerViewBase::tearDownScreen();
}

void ScreenExtChargerView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

namespace
{
const char* chem_name(uint8_t chemistry)
{
    switch (chemistry)
    {
    case CHEM_LIION: return "LI-ION";
    case CHEM_LIION_HV: return "LI-ION HV";
    case CHEM_LIFEPO4: return "LIFEPO4";
    case CHEM_LTO: return "LTO";
    case CHEM_LEAD: return "LEAD";
    case CHEM_NIMH: return "NIMH";
    default: return "CUSTOM";
    }
}

const char* state_face(PsuChgState state)
{
    switch (state)
    {
    case CHG_IDLE: return "IDLE";
    case CHG_VALIDATE: return "VALIDATE";
    case CHG_WAIT: return "WAIT";
    case CHG_PRECHARGE: return "PRECHARGE";
    case CHG_CC: return "CC";
    case CHG_CV: return "CV";
    case CHG_ABSORPTION: return "ABSORB";
    case CHG_FLOAT: return "FLOAT";
    case CHG_TERMINATING: return "ENDING";
    case CHG_COMPLETE: return "COMPLETE";
    case CHG_PAUSED: return "PAUSED";
    case CHG_ABORTED: return "ABORTED";
    default: return "FAULT";
    }
}

touchgfx::colortype state_color(PsuChgState state)
{
    if (state == CHG_FAULT || state == CHG_ABORTED)
        return lab_red();
    if (state == CHG_CC || state == CHG_PRECHARGE)
        return lab_amber();
    if (state == CHG_COMPLETE || state == CHG_IDLE || state == CHG_FLOAT)
        return lab_green();
    return lab_cyan();
}
}

void ScreenExtChargerView::refresh()
{
    PsuCharger* chg;
    char buf[24];
    uint32_t pack_mv;
    psu_app_ensure();
    chg = psu_charger();
    lab_show(ChgState, ChgStateBuffer, CHGSTATE_SIZE, state_face(chg->state), state_color(chg->state));
    lab_show(ChgNote, ChgNoteBuffer, CHGNOTE_SIZE,
             chg->reason[0] ? chg->reason : "SELECT A PROFILE",
             (chg->state == CHG_FAULT) ? lab_red() : lab_muted());
    lab_show(ChgChem, ChgChemBuffer, CHGCHEM_SIZE, chem_name(chg->profile.chemistry), lab_text());
    pack_mv = chg->profile.cv_mv_cell * (chg->profile.cells ? chg->profile.cells : 1U);
    psu_format_voltage(buf, sizeof(buf), pack_mv);
    lab_show(ChgVolt, ChgVoltBuffer, CHGVOLT_SIZE, buf, lab_cyan());
    psu_format_current_ma(buf, sizeof(buf), chg->profile.cc_ma);
    lab_show(ChgAmp, ChgAmpBuffer, CHGAMP_SIZE, buf, lab_amber());
}

static uint8_t chg_armed;
static uint8_t chg_chem;

void ScreenExtChargerView::chgProfile()
{
    PsuChgProfile profile;
    chg_chem = (uint8_t)((chg_chem + 1U) % 7U);
    psu_chg_profile_defaults(&profile, chg_chem);
    profile.cells = 1U;
    profile.confirmed = 0U;
    profile.polarity_checked = 0U;
    psu_charger()->profile = profile;
    chg_armed = 1U;
    refresh();
}

void ScreenExtChargerView::chgStart()
{
    PsuChgProfile profile = psu_charger()->profile;
    PsuChgSense sense;
    if (!chg_armed)
    {
        refresh();
        return;
    }
    profile.confirmed = 1U;
    profile.polarity_checked = 1U;
    if (profile.chemistry == CHEM_CUSTOM)
        profile.custom_unlocked = 1U;
    memset(&sense, 0, sizeof(sense));
    {
        PsuSnapshot live;
        psu_snapshot(&live);
        sense.telemetry_ok = (live.g0_connected && live.current_valid) ? 1U : 0U;
        sense.permit = live.fault_latched ? 0U : 1U;
        sense.pack_mv = live.vout_mv;
        sense.temp_centi = live.mos_centi;
    }
    psu_chg_start(psu_charger(), &profile, &sense, psu_app_now());
    refresh();
}

void ScreenExtChargerView::chgAbort()
{
    psu_chg_abort(psu_charger(), "Operator abort", psu_app_now());
    chg_armed = 0U;
    refresh();
}
