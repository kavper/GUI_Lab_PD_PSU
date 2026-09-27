#ifndef SCREENMEASUREMENTSVIEW_HPP
#define SCREENMEASUREMENTSVIEW_HPP

#include <gui_generated/screenmeasurements_screen/ScreenMeasurementsViewBase.hpp>
#include <gui/screenmeasurements_screen/ScreenMeasurementsPresenter.hpp>
#include <stdint.h>

class ScreenMeasurementsView : public ScreenMeasurementsViewBase
{
public:
    ScreenMeasurementsView();
    virtual ~ScreenMeasurementsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

protected:
    uint8_t divider;
    void refresh();
};

#endif
