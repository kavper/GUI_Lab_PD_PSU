#ifndef SCREENMEASUREMENTSVIEW_HPP
#define SCREENMEASUREMENTSVIEW_HPP

#include <gui_generated/screenmeasurements_screen/ScreenMeasurementsViewBase.hpp>
#include <gui/screenmeasurements_screen/ScreenMeasurementsPresenter.hpp>
#include <stdint.h>
#include <gui/common/LabSwipe.hpp>

class ScreenMeasurementsView : public ScreenMeasurementsViewBase
{
public:
    ScreenMeasurementsView();
    virtual ~ScreenMeasurementsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
    virtual void handleTickEvent();

protected:
    LabSwipe swipe;
    uint8_t divider;
    void refresh();
};

#endif
