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
    , chemistry(0)
    , chem_set(0)
    , cells_set(0)
    , polarity_latched(0)
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
    char chem[24];
    const char* note;
    psu_app_ensure();
    chg = psu_charger();
    lab_show(ChgState, ChgStateBuffer, CHGSTATE_SIZE, state_face(chg->state), state_color(chg->state));
    note = chg->reason[0] ? chg->reason : "SELECT A PROFILE";
    if (!chem_set && strcmp(note, "IDLE") == 0)
        note = "SELECT A PROFILE";
    lab_show(ChgNote, ChgNoteBuffer, CHGNOTE_SIZE, note,
             (chg->state == CHG_FAULT) ? lab_red() : (polarity_latched ? lab_amber() : lab_muted()));
    if (!chem_set)
        lab_show(ChgChem, ChgChemBuffer, CHGCHEM_SIZE, "NONE", lab_muted());
    else if (!cells_set)
        lab_show(ChgChem, ChgChemBuffer, CHGCHEM_SIZE, chem_name(chg->profile.chemistry), lab_text());
    else
    {
        snprintf(chem, sizeof(chem), "%s %uS", chem_name(chg->profile.chemistry), chg->profile.cells);
        lab_show(ChgChem, ChgChemBuffer, CHGCHEM_SIZE, chem, lab_text());
    }
    if (!cells_set)
        lab_show(ChgVolt, ChgVoltBuffer, CHGVOLT_SIZE, "--", lab_muted());
    else
    {
        psu_format_voltage(buf, sizeof(buf),
                           chg->profile.cv_mv_cell * (uint32_t)chg->profile.cells);
        lab_show(ChgVolt, ChgVoltBuffer, CHGVOLT_SIZE, buf, lab_cyan());
    }
    if (!chem_set)
        lab_show(ChgAmp, ChgAmpBuffer, CHGAMP_SIZE, "--", lab_muted());
    else
    {
        psu_format_current_ma(buf, sizeof(buf), chg->profile.cc_ma);
        lab_show(ChgAmp, ChgAmpBuffer, CHGAMP_SIZE, buf, lab_amber());
    }
}

static void read_sense(PsuChgSense* sense)
{
    PsuSnapshot live;
    memset(sense, 0, sizeof(*sense));
    psu_snapshot(&live);
    sense->telemetry_ok = (live.g0_connected && live.current_valid) ? 1U : 0U;
    sense->permit = live.fault_latched ? 0U : 1U;
    sense->pack_mv = live.vout_mv;
    sense->temp_centi = live.mos_centi;
    sense->reverse_hw = 0U;
}

void ScreenExtChargerView::chgProfile()
{
    PsuChgProfile profile;
    PsuCharger* chg = psu_charger();
    if (chg->running)
    {
        snprintf(chg->reason, sizeof(chg->reason), "STOP BEFORE CHANGING PROFILE");
        refresh();
        return;
    }
    chemistry = (uint8_t)((chemistry + 1U) % 7U);
    psu_chg_profile_defaults(&profile, chemistry);
    profile.cells = 0U;
    profile.confirmed = 0U;
    profile.polarity_checked = 0U;
    profile.custom_unlocked = 0U;
    chg->profile = profile;
    chem_set = 1U;
    cells_set = 0U;
    polarity_latched = 0U;
    snprintf(chg->reason, sizeof(chg->reason), "CHOOSE CELL COUNT");
    refresh();
}

void ScreenExtChargerView::chgCells()
{
    PsuCharger* chg = psu_charger();
    PsuChgProfile profile = chg->profile;
    uint8_t max_cells;
    uint8_t next;
    if (chg->running)
    {
        snprintf(chg->reason, sizeof(chg->reason), "STOP BEFORE CHANGING PROFILE");
        refresh();
        return;
    }
    if (!chem_set)
    {
        snprintf(chg->reason, sizeof(chg->reason), "SELECT A PROFILE");
        refresh();
        return;
    }
    max_cells = psu_chg_max_cells(&profile);
    if (max_cells == 0U)
    {
        snprintf(chg->reason, sizeof(chg->reason), "CELL COUNT REQUIRED");
        refresh();
        return;
    }
    next = (uint8_t)(profile.cells + 1U);
    if (!cells_set || next < 1U || next > max_cells)
        next = 1U;
    profile.cells = next;
    profile.confirmed = 0U;
    profile.polarity_checked = 0U;
    chg->profile = profile;
    cells_set = 1U;
    polarity_latched = 0U;
    snprintf(chg->reason, sizeof(chg->reason), "CONFIRM POLARITY, THEN START");
    refresh();
}

void ScreenExtChargerView::chgStart()
{
    PsuChgProfile profile = psu_charger()->profile;
    PsuChgSense sense;
    int step;
    read_sense(&sense);
    step = psu_chg_user_start(psu_charger(), &profile, &sense, psu_app_now(),
                              chem_set, cells_set, polarity_latched);
    if (step == 1)
        polarity_latched = 1U;
    refresh();
}

void ScreenExtChargerView::chgAbort()
{
    psu_chg_abort(psu_charger(), "Operator abort", psu_app_now());
    chem_set = 0U;
    cells_set = 0U;
    polarity_latched = 0U;
    refresh();
}
