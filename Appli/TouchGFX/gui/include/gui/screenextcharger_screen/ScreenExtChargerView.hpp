#ifndef SCREENEXTCHARGERVIEW_HPP
#define SCREENEXTCHARGERVIEW_HPP

#include <gui_generated/screenextcharger_screen/ScreenExtChargerViewBase.hpp>
#include <gui/screenextcharger_screen/ScreenExtChargerPresenter.hpp>
#include <stdint.h>

class ScreenExtChargerView : public ScreenExtChargerViewBase
{
public:
    ScreenExtChargerView();
    virtual ~ScreenExtChargerView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void chgProfile();
    virtual void chgStart();
    virtual void chgAbort();

protected:
    uint8_t divider;
    void refresh();
};

#endif
