#ifndef SCREENBATTERYVIEW_HPP
#define SCREENBATTERYVIEW_HPP
#include <gui_generated/screenbattery_screen/ScreenBatteryViewBase.hpp>
#include <gui/screenbattery_screen/ScreenBatteryPresenter.hpp>
#include <stdint.h>
class ScreenBatteryView : public ScreenBatteryViewBase {
public:
    ScreenBatteryView();
    virtual ~ScreenBatteryView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void refreshTelemetry();
protected:
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
};
#endif
