#include <gui/screendiagnostics_screen/ScreenDiagnosticsView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_format.h"
}

ScreenDiagnosticsView::ScreenDiagnosticsView()
    : refreshDivider(0)
{

}

void ScreenDiagnosticsView::setupScreen()
{
    ScreenDiagnosticsViewBase::setupScreen();
    handleTickEvent();
}

void ScreenDiagnosticsView::handleTickEvent()
{
    if (++refreshDivider < 10)
        return;
    refreshDivider = 0;

    {
        PsuSnapshot face;
        char reading[24];
        psu_app_ensure();
        psu_snapshot(&face);
        lab_show(DiagLink, DiagLinkBuffer, DIAGLINK_SIZE,
                 face.g0_connected ? (face.g0_stale ? "STALE" : "ONLINE") : "OFFLINE",
                 face.g0_connected ? (face.g0_stale ? lab_amber() : lab_green()) : lab_muted());
        psu_format_voltage(reading, sizeof(reading), face.vout_mv);
        lab_show(DiagVolt, DiagVoltBuffer, DIAGVOLT_SIZE, reading, lab_cyan());
        psu_format_current_ua(reading, sizeof(reading), face.display_current_ua);
        lab_show(DiagAmp, DiagAmpBuffer, DIAGAMP_SIZE, reading, face.mode_cc ? lab_amber() : lab_green());
        lab_show(DiagFault, DiagFaultBuffer, DIAGFAULT_SIZE,
                 face.fault_latched ? "FAULT" : "NONE",
                 face.fault_latched ? lab_red() : lab_green());
    }

    {
        char ascii[400];
        psu_render_diagnostics(ascii, sizeof(ascii));
        lab_show(DiagnosticsValues, DiagnosticsValuesBuffer, DIAGNOSTICSVALUES_SIZE,
                 ascii, lab_text());
    }
}

void ScreenDiagnosticsView::tearDownScreen()
{
    ScreenDiagnosticsViewBase::tearDownScreen();
}

void ScreenDiagnosticsView::handleClickEvent(const touchgfx::ClickEvent& event)
{
    swipe.onClick(event, false);
    ScreenDiagnosticsViewBase::handleClickEvent(event);
}

void ScreenDiagnosticsView::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    if (swipe.onGesture(event))
    {
        application().gotoScreenSettingsScreenWipeTransitionEast();
        return;
    }
    ScreenDiagnosticsViewBase::handleGestureEvent(event);
}
