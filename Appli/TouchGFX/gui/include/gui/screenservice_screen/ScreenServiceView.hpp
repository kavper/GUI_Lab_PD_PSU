#ifndef SCREENSERVICEVIEW_HPP
#define SCREENSERVICEVIEW_HPP

#include <gui_generated/screenservice_screen/ScreenServiceViewBase.hpp>
#include <gui/screenservice_screen/ScreenServicePresenter.hpp>
#include <stdint.h>

class ScreenServiceView : public ScreenServiceViewBase
{
public:
    void setupTheme();
    ScreenServiceView();
    virtual ~ScreenServiceView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void serviceToggle();
    virtual void serviceHelp();
    virtual void serviceStatus();
    virtual void allOff();
protected:
    uint8_t divider;
    void refresh();
};

#endif
