#ifndef SCREENAPPEARANCEVIEW_HPP
#define SCREENAPPEARANCEVIEW_HPP

#include <gui_generated/screenappearance_screen/ScreenAppearanceViewBase.hpp>
#include <gui/screenappearance_screen/ScreenAppearancePresenter.hpp>
#include <gui/common/LabSwipe.hpp>

class ScreenAppearanceView : public ScreenAppearanceViewBase
{
public:
    ScreenAppearanceView();
    virtual ~ScreenAppearanceView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
protected:
    LabSwipe swipe;
    void refresh();
};

#endif
