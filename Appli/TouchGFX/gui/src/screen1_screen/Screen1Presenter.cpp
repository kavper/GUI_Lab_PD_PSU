#include <gui/screen1_screen/Screen1View.hpp>
#include <stdio.h>
#include <gui/screen1_screen/Screen1Presenter.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_limits.h"
}

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v), lastRequestedMv(0)
{

}

void Screen1Presenter::activate()
{
    outputVoltageFilter.reset();
    inputVoltageFilter.reset();
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
    PsuSnapshot live;
    psu_snapshot(&live);
    const bool fresh = live.g0_connected && !live.g0_stale && live.current_valid;
    // Use values and acquisition identity from the same snapshot. GUI ticks
    // cannot turn one UART measurement into several samples of the average.
    if (lastRequestedMv != live.requested_mv) outputVoltageFilter.reset();
    lastRequestedMv = live.requested_mv;
    const bool changing = live.output_phase == G4_OUTPUT_STARTING || live.output_phase == G4_OUTPUT_STOPPING;
    const uint32_t shownMv = outputVoltageFilter.update(live.vout_mv, live.meter_ms, live.meter_serial, fresh, changing);
    const uint32_t shownInputMv = inputVoltageFilter.update(live.vin_mv, live.meter_ms, live.meter_serial, fresh);
    (void)inputVoltageMv;
    view.setMeasurements(shownMv, shownUa, mosfetDeciC);
    view.setInputMetrics(shownInputMv, psu_output_power_mw(voltageMv, shownUa));
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
