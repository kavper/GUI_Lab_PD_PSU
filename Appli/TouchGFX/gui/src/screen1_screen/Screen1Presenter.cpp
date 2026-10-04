#include <gui/screen1_screen/Screen1View.hpp>
#include <stdio.h>
#include <gui/screen1_screen/Screen1Presenter.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_limits.h"
}

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{

}

void Screen1Presenter::activate()
{
    // Prepare real values before the transition clears setup invalidations.
    // They remain stable throughout the reveal instead of painting over splash.
    PsuSnapshot snap;
    psu_snapshot(&snap);
    ldoTelemetryUpdated(snap.vin_mv, snap.vout_mv, snap.signed_current_ua,
        static_cast<int16_t>(snap.mos_centi / 10), static_cast<int16_t>(snap.pcb_centi / 10),
        snap.mode_cc ? 2U : 1U, snap.g0_connected != 0,
        snap.output_confirmed != 0, snap.current_valid != 0,
        snap.current_valid != 0);
}

void Screen1Presenter::deactivate()
{

}

void Screen1Presenter::ldoTelemetryUpdated(uint32_t inputVoltageMv,
                                          uint32_t voltageMv, int32_t currentUa,
                                          int16_t mosfetDeciC, int16_t pcbDeciC,
                                          uint8_t mode, bool connected, bool outputRequested,
                                          bool currentValid, bool currentCalibrated)
{
    const int32_t shownUa = psu_display_current_ua(currentUa, currentValid ? 1 : 0);
    view.setMeasurements(voltageMv, shownUa, mosfetDeciC);
    view.setInputMetrics(inputVoltageMv, psu_output_power_mw(voltageMv, shownUa));
    view.setPcbTemperature(pcbDeciC);
    view.setRegulationMode(mode == 2);
    view.setCurrentMeasurementCalibrated(currentValid && currentCalibrated);
    view.setControllerOutputState(connected && outputRequested);
    {
        PsuSnapshot snap;
        char link[24];
        psu_snapshot(&snap);
        view.syncControllerSetpoints(snap.requested_mv, snap.requested_ma);
        view.setTemperaturesDeciC(snap.mos_centi / 10, snap.pcb_centi / 10);
        view.setTelemetryAvailable(connected && !snap.g0_stale && currentValid, currentValid, snap.temperature_valid);
        if(snap.g4_uart_configured)view.setHostAuxMetrics();
        if (snap.g4_uart_configured && snap.g4_link != G4_LINK_ONLINE) snprintf(link, sizeof(link), "G4 OFFLINE");
        else if (!connected) snprintf(link, sizeof(link), "G0 OFFLINE");
        else if (snap.g0_stale) snprintf(link, sizeof(link), "G0 STALE");
        else
            snprintf(link, sizeof(link), snap.g4_uart_configured ? "G4 ONLINE" : "G0 ONLINE");
        view.setLinkStatus(link);
    }
}
