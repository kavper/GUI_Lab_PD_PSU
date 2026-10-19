#ifndef SCREENSERVICEVIEW_HPP
#define SCREENSERVICEVIEW_HPP

#include <gui_generated/screenservice_screen/ScreenServiceViewBase.hpp>
#include <gui/screenservice_screen/ScreenServicePresenter.hpp>
#include <stdint.h>
#include <gui/common/LabSwipe.hpp>

class ScreenServiceView : public ScreenServiceViewBase
{
public:
    ScreenServiceView();
    virtual ~ScreenServiceView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
    virtual void handleTickEvent();
    virtual void serviceToggle();
    virtual void serviceHelp();
    virtual void serviceStatus();
protected:
    LabSwipe swipe;
    uint8_t divider;
    void refresh();
};

#endif
