#include <gui/screensplash_screen/ScreenSplashView.hpp>

ScreenSplashView::ScreenSplashView() :
    splashTicks(0)
{

}

void ScreenSplashView::setupScreen()
{
    ScreenSplashViewBase::setupScreen();
    SplashProgressFill.setWidth(1);
}

void ScreenSplashView::handleTickEvent()
{
    if (splashTicks < 120)
    {
        ++splashTicks;
        const int16_t newWidth = static_cast<int16_t>((520UL * splashTicks) / 120UL);
        SplashProgressFill.invalidate();
        SplashProgressFill.setWidth(newWidth > 0 ? newWidth : 1);
        SplashProgressFill.invalidate();
    }
    else
    {
        application().gotoScreen1ScreenWipeTransitionEast();
    }
}

void ScreenSplashView::tearDownScreen()
{
    ScreenSplashViewBase::tearDownScreen();
}
