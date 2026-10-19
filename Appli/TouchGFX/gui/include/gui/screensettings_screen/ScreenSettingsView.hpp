#ifndef SCREENSETTINGSVIEW_HPP
#define SCREENSETTINGSVIEW_HPP

#include <gui_generated/screensettings_screen/ScreenSettingsViewBase.hpp>
#include <gui/screensettings_screen/ScreenSettingsPresenter.hpp>
#include <gui/common/LabSwipe.hpp>

class ScreenSettingsView : public ScreenSettingsViewBase
{
public:
    ScreenSettingsView();
    virtual ~ScreenSettingsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
protected:
    LabSwipe swipe;
};

#endif // SCREENSETTINGSVIEW_HPP
