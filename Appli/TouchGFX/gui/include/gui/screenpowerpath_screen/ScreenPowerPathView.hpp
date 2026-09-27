#ifndef SCREENPOWERPATHVIEW_HPP
#define SCREENPOWERPATHVIEW_HPP

#include <gui_generated/screenpowerpath_screen/ScreenPowerPathViewBase.hpp>
#include <gui/screenpowerpath_screen/ScreenPowerPathPresenter.hpp>
#include <stdint.h>

class ScreenPowerPathView : public ScreenPowerPathViewBase
{
public:
    ScreenPowerPathView();
    virtual ~ScreenPowerPathView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void pathPermit();
    virtual void pathRemote();
protected:
    uint8_t divider;
    void refresh();
};

#endif
