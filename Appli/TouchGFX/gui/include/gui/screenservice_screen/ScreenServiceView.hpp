#ifndef SCREENSERVICEVIEW_HPP
#define SCREENSERVICEVIEW_HPP
#include <gui_generated/screenservice_screen/ScreenServiceViewBase.hpp>
#include <gui/screenservice_screen/ScreenServicePresenter.hpp>
#include <gui/common/Kit.hpp>
#include <stdint.h>
// Service: controller console (recent commands / replies) and lock controls.
class ScreenServiceView : public ScreenServiceViewBase
{
public:
    ScreenServiceView();
    virtual ~ScreenServiceView() {}
    virtual void setupScreen();
    virtual void tearDownScreen() { ScreenServiceViewBase::tearDownScreen(); }
    virtual void handleTickEvent();
    virtual void serviceToggle();
    virtual void serviceHelp();
    virtual void serviceStatus();
    virtual void allOff();
protected:
    uint8_t divider;
    char notice[80];
    void refresh();
    void layout();
    void onClick(const kit::Btn& b);
    void result(bool accepted);
    kit::Header header;
    kit::Card consoleCard, lockCard;
    kit::Rows console;
    kit::Label consoleNote, lockState, lockSub;
    kit::Btn lockIcon, toggleBtn, statusBtn, helpBtn, clearBtn;
    touchgfx::Callback<ScreenServiceView, const kit::Btn&> clicked;
};
#endif
