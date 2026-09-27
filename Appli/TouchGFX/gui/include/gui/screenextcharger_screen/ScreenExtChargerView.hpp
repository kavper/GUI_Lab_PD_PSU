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
    virtual void chgCells();
    virtual void chgStart();
    virtual void chgAbort();

protected:
    uint8_t divider;
    uint8_t chemistry;
    uint8_t chem_set;
    uint8_t cells_set;
    uint8_t polarity_latched;
    void refresh();
};

#endif
