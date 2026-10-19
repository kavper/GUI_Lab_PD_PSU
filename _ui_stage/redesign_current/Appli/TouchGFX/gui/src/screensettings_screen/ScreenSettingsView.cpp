#include <gui/screensettings_screen/ScreenSettingsView.hpp>

ScreenSettingsView::ScreenSettingsView()
{

}

void ScreenSettingsView::setupScreen()
{
    ScreenSettingsViewBase::setupScreen();
}

void ScreenSettingsView::tearDownScreen()
{
    ScreenSettingsViewBase::tearDownScreen();
}

void ScreenSettingsView::handleClickEvent(const touchgfx::ClickEvent& event)
{
    swipe.onClick(event, false);
    ScreenSettingsViewBase::handleClickEvent(event);
}

void ScreenSettingsView::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    if (swipe.onGesture(event))
    {
        application().gotoScreen1ScreenWipeTransitionEast();
        return;
    }
    ScreenSettingsViewBase::handleGestureEvent(event);
}
