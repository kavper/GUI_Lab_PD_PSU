#ifndef SCREENPOWERPATHVIEW_HPP
#define SCREENPOWERPATHVIEW_HPP
#include <gui_generated/screenpowerpath_screen/ScreenPowerPathViewBase.hpp>
#include <gui/screenpowerpath_screen/ScreenPowerPathPresenter.hpp>
#include <stdint.h>
class ScreenPowerPathView : public ScreenPowerPathViewBase {
public:
    void setupTheme();
    ScreenPowerPathView();
    virtual ~ScreenPowerPathView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void togglePermit();
    virtual void toggleSense();
    virtual void refreshTelemetry();
protected:
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
};
#endif
