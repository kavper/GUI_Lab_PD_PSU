#ifndef SCREENBATTERYVIEW_HPP
#define SCREENBATTERYVIEW_HPP

#include <gui_generated/screenbattery_screen/ScreenBatteryViewBase.hpp>
#include <gui/screenbattery_screen/ScreenBatteryPresenter.hpp>
#include <gui/common/LabSwipe.hpp>
#include <stdint.h>

class ScreenBatteryView : public ScreenBatteryViewBase
{
public:
    ScreenBatteryView();
    virtual ~ScreenBatteryView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);

protected:
    uint8_t divider;
    LabSwipe swipe;
    void refresh();
};

#endif
