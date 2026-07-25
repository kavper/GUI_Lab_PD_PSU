#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

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
    (void)outputRequested;
    view.setMeasurements(voltageMv, currentUa, mosfetDeciC);
    const uint32_t positiveCurrentUa = currentUa > 0 ? static_cast<uint32_t>(currentUa) : 0U;
    const uint32_t outputPowerMw =
        static_cast<uint32_t>((static_cast<uint64_t>(voltageMv) * positiveCurrentUa) / 1000000U);
    view.setInputMetrics(inputVoltageMv, outputPowerMw);
    view.setPcbTemperature(pcbDeciC);
    view.setRegulationMode(mode == 2);
    view.setCurrentMeasurementCalibrated(currentValid && currentCalibrated);
    if (!connected)
        view.setControllerOutputState(false);
}
