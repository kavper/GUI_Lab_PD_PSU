#ifndef SCREENBATTERYVIEW_HPP
#define SCREENBATTERYVIEW_HPP

#include <gui_generated/screenbattery_screen/ScreenBatteryViewBase.hpp>
#include <gui/screenbattery_screen/ScreenBatteryPresenter.hpp>
#include <stdint.h>

class ScreenBatteryView : public ScreenBatteryViewBase
{
public:
    ScreenBatteryView();
    virtual ~ScreenBatteryView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

protected:
    uint8_t divider;
    void refresh();
};

#endif
