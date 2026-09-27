#include <gui/screenextcharger_screen/ScreenExtChargerView.hpp>
#include <touchgfx/Unicode.hpp>
#include <string.h>
extern "C" {
#include "psu_app.h"
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

void ScreenExtChargerView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_charger(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), ChgBodyBuffer, CHGBODY_SIZE);
    ChgBody.invalidate();
}



#include "psu_charger.h"

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
