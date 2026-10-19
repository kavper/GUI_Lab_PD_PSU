#include <gui/screensplash_screen/ScreenSplashView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_version.h"
}

ScreenSplashView::ScreenSplashView() :
    splashTicks(0)
{

}

void ScreenSplashView::setupScreen()
{
    ScreenSplashViewBase::setupScreen();
    SplashLogo.setAlpha(0);
    SplashProgressFill.setWidth(1);
    touchgfx::Unicode::snprintf(SplashBuildBuffer, SPLASHBUILD_SIZE, "FW %s  %s",
                               PSU_FW_VERSION, PSU_FW_DATE);
    SplashBuild.invalidate();
}

void ScreenSplashView::handleTickEvent()
{
    if (splashTicks < 120)
    {
        ++splashTicks;
        const uint16_t alpha = (splashTicks >= 32U)
            ? 255U
            : static_cast<uint16_t>((255U * splashTicks) / 32U);
        const int16_t newWidth = static_cast<int16_t>((512UL * splashTicks) / 120UL);
        SplashLogo.setAlpha(alpha);
        SplashLogo.invalidate();
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
