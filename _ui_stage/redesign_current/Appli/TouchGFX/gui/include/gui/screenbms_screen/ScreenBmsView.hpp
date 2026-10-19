#ifndef SCREENBMSVIEW_HPP
#define SCREENBMSVIEW_HPP

#include <gui_generated/screenbms_screen/ScreenBmsViewBase.hpp>
#include <gui/screenbms_screen/ScreenBmsPresenter.hpp>
#include <gui/common/LabSwipe.hpp>
#include <stdint.h>

class ScreenBmsView : public ScreenBmsViewBase
{
public:
    ScreenBmsView();
    virtual ~ScreenBmsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
    virtual void bmsConfigure();
    virtual void bmsOff();
    virtual void bmsClear();
    virtual void bmsRefresh();
protected:
    uint8_t divider;
    LabSwipe swipe;
    void refresh();
};

#endif
