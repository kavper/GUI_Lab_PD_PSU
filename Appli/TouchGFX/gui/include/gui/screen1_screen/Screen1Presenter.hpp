#ifndef SCREEN1PRESENTER_HPP
#define SCREEN1PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>
#include <gui/common/DisplayVoltageFilter.hpp>

using namespace touchgfx;

class Screen1View;

class Screen1Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen1Presenter(Screen1View& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();
    virtual void ldoTelemetryUpdated(uint32_t inputVoltageMv,
                                     uint32_t voltageMv, int32_t currentUa,
                                     int16_t mosfetDeciC, int16_t pcbDeciC,
                                     uint8_t mode, bool connected, bool outputRequested,
                                     bool currentValid, bool currentCalibrated);

    virtual ~Screen1Presenter() {}

private:
    Screen1Presenter();

    Screen1View& view;
    DisplayVoltageFilter outputVoltageFilter;
    DisplayVoltageFilter inputVoltageFilter;
    uint32_t lastRequestedMv;
};

#endif // SCREEN1PRESENTER_HPP
