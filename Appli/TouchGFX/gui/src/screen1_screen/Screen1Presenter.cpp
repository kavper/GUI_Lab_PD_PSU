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
        if (snap.g4_link == G4_LINK_ONLINE)
            snprintf(link, sizeof(link), "G4 ONLINE");
        else if (snap.g4_link == G4_LINK_STALE)
            snprintf(link, sizeof(link), "G4 STALE");
        else
            snprintf(link, sizeof(link), "G4 OFFLINE");
        view.setLinkStatus(link);
    }
}
