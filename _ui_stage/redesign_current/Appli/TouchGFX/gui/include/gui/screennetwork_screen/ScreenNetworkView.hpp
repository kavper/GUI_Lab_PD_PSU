#ifndef SCREENNETWORKVIEW_HPP
#define SCREENNETWORKVIEW_HPP

#include <gui_generated/screennetwork_screen/ScreenNetworkViewBase.hpp>
#include <gui/screennetwork_screen/ScreenNetworkPresenter.hpp>
#include <stdint.h>
#include <gui/common/LabSwipe.hpp>

class ScreenNetworkView : public ScreenNetworkViewBase
{
public:
    ScreenNetworkView();
    virtual ~ScreenNetworkView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
    virtual void handleTickEvent();

protected:
    LabSwipe swipe;
    uint8_t divider;
    void refresh();
};

#endif
