#ifndef SCREENBATTERYVIEW_HPP
#define SCREENBATTERYVIEW_HPP
#include <gui_generated/screenbattery_screen/ScreenBatteryViewBase.hpp>
#include <gui/screenbattery_screen/ScreenBatteryPresenter.hpp>
#include <stdint.h>
#include <gui/common/BatteryTrendWidget.hpp>
class ScreenBatteryView : public ScreenBatteryViewBase {
public:
    void setupTheme();
    ScreenBatteryView();
    virtual ~ScreenBatteryView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void refreshTelemetry();
    virtual void showBattery();
    virtual void openBms();
    virtual void showCharger();
    virtual void showEnergy();
    virtual void openUsb();
protected:
    friend class BatterySelfTest;
    BatteryTrendWidget trend;
    unsigned page;
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
};
#endif
