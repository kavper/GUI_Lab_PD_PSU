#ifndef SCREENBMSVIEW_HPP
#define SCREENBMSVIEW_HPP

#include <gui_generated/screenbms_screen/ScreenBmsViewBase.hpp>
#include <gui/screenbms_screen/ScreenBmsPresenter.hpp>
#include <stdint.h>

class ScreenBmsView : public ScreenBmsViewBase
{
public:
    ScreenBmsView();
    virtual ~ScreenBmsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void bmsConfigure();
    virtual void bmsOff();
    virtual void bmsClear();
    virtual void bmsRefresh();
protected:
    uint8_t divider;
    void refresh();
};

#endif
