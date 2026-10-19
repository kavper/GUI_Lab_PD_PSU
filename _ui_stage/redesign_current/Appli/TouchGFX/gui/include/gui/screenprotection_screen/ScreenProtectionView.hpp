#ifndef SCREENPROTECTIONVIEW_HPP
#define SCREENPROTECTIONVIEW_HPP

#include <gui_generated/screenprotection_screen/ScreenProtectionViewBase.hpp>
#include <gui/screenprotection_screen/ScreenProtectionPresenter.hpp>
#include <stdint.h>
#include <gui/common/LabSwipe.hpp>

class ScreenProtectionView : public ScreenProtectionViewBase
{
public:
    ScreenProtectionView();
    virtual ~ScreenProtectionView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
    virtual void handleTickEvent();
    virtual void clearFault();
protected:
    LabSwipe swipe;
    uint8_t divider;
    void refresh();
};

#endif
