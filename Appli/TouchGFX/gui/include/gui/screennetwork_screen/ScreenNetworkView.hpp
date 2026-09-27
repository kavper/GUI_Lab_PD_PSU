#ifndef SCREENNETWORKVIEW_HPP
#define SCREENNETWORKVIEW_HPP

#include <gui_generated/screennetwork_screen/ScreenNetworkViewBase.hpp>
#include <gui/screennetwork_screen/ScreenNetworkPresenter.hpp>
#include <stdint.h>

class ScreenNetworkView : public ScreenNetworkViewBase
{
public:
    ScreenNetworkView();
    virtual ~ScreenNetworkView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

protected:
    uint8_t divider;
    void refresh();
};

#endif
