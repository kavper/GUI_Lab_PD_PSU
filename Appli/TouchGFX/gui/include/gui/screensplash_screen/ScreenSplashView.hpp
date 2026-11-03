#pragma once
#include <gui_generated/screensplash_screen/ScreenSplashViewBase.hpp>
#include <gui/screensplash_screen/ScreenSplashPresenter.hpp>
#include <gui/common/Kit.hpp>
class ScreenSplashView:public ScreenSplashViewBase{
public:
 ScreenSplashView():ticks(0),leaving(false){}
 virtual void setupScreen();virtual void handleTickEvent();
 virtual void continueStartup();
private:
 unsigned ticks;bool leaving;
 static const int ROWS=5;
 kit::Fill side, sideEdge, rowLine[ROWS];
 kit::Label checkCaption, lockNote, rowName[ROWS], rowState[ROWS], title, subtitle, specs, build1, build2, hint;
 kit::Btn rowMark[ROWS];
 void layout();
 void refreshChecks();
};
