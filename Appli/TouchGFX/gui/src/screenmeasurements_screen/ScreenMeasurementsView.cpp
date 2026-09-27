#include <gui/screenmeasurements_screen/ScreenMeasurementsView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_format.h"
}

ScreenMeasurementsView::ScreenMeasurementsView()
    : divider(0)
{
}

void ScreenMeasurementsView::setupScreen()
{
    ScreenMeasurementsViewBase::setupScreen();
    refresh();
}

void ScreenMeasurementsView::tearDownScreen()
{
    ScreenMeasurementsViewBase::tearDownScreen();
}

void ScreenMeasurementsView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenMeasurementsView::refresh()
{
    PsuSnapshot snap;
    char buf[24];
    touchgfx::colortype mode_color;
    psu_app_ensure();
    psu_snapshot(&snap);
    psu_format_voltage(buf, sizeof(buf), snap.vout_mv);
    lab_show(MeasVolt, MeasVoltBuffer, MEASVOLT_SIZE, buf, lab_cyan());
    psu_format_current_ua(buf, sizeof(buf), snap.display_current_ua);
    lab_show(MeasAmp, MeasAmpBuffer, MEASAMP_SIZE, buf, snap.mode_cc ? lab_amber() : lab_green());
    psu_format_power_mw(buf, sizeof(buf), snap.power_mw);
    lab_show(MeasPower, MeasPowerBuffer, MEASPOWER_SIZE, buf, lab_cyan());
    psu_format_voltage(buf, sizeof(buf), snap.vin_mv);
    lab_show(MeasVin, MeasVinBuffer, MEASVIN_SIZE, buf, lab_text());
    psu_format_centi_c(buf, sizeof(buf), snap.mos_centi);
    lab_show(MeasTemp, MeasTempBuffer, MEASTEMP_SIZE, buf, lab_text());
    if (snap.output_confirmed)
        mode_color = lab_red();
    else if (snap.mode_cc)
        mode_color = lab_amber();
    else
        mode_color = lab_cyan();
    lab_show(MeasMode, MeasModeBuffer, MEASMODE_SIZE, snap.mode_cc ? "CC" : "CV", mode_color);
}
