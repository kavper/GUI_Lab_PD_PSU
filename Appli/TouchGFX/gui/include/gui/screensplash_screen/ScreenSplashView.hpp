#pragma once
#include <gui_generated/screensplash_screen/ScreenSplashViewBase.hpp>
#include <gui/screensplash_screen/ScreenSplashPresenter.hpp>
class ScreenSplashView:public ScreenSplashViewBase{
public:
 ScreenSplashView():ticks(0){}
 virtual void setupScreen();virtual void handleTickEvent();
 virtual void tearDownScreen(){ScreenSplashViewBase::tearDownScreen();}
 virtual void continueStartup();
private:unsigned ticks;
};
