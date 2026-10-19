#include <gui/screensplash_screen/ScreenSplashView.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/UiTheme.hpp>
extern "C" {
#include "psu_version.h"
}
void ScreenSplashView::setupScreen(){
 ticks=0;leaving=false;ScreenSplashViewBase::setupScreen();
 auto& theme=ui::ThemeScreen::get();theme.begin(*this);
 theme.box(SplashBackground,ui::BACKGROUND);theme.box(StartupTrack,ui::BORDER);
 theme.text(SplashTitle,ui::TEXT);theme.text(SplashSubtitle,ui::TEXT);
 theme.text(SplashThesis,ui::MUTED);theme.text(SplashBuild,ui::MUTED);
 theme.text(Checks,ui::TEXT);theme.text(StartupHint,ui::MUTED);
 theme.button(ContinueButton,ui::PRIMARY);theme.apply();
 char b[80];snprintf(b,sizeof(b),"FW %s / %s",PSU_FW_VERSION,PSU_FW_DATE);
 lab_show(SplashBuild,SplashBuildBuffer,SPLASHBUILD_SIZE,b,lab_muted());handleTickEvent();
}
void ScreenSplashView::handleTickEvent(){
 if(leaving)return;
 ++ticks;
 if(ticks%3==1){StartupProgress.invalidate();StartupProgress.setWidth(ticks>=120?560:ticks*560/120);StartupProgress.setColor(ui::Theme::color(ui::ACCENT));StartupProgress.invalidate();}
 if(ticks%12==1){
  TelemetryRecord t(0),tb(1),tc(2);char b[220];const char *missing=ticks>80?"NOT CONNECTED":"CONNECTING";
  snprintf(b,sizeof(b),"SUPERVISOR / OUTPUT    %s\nBATTERY / BMS          %s\nUSB-C / POWER DELIVERY %s",t.fresh?"ONLINE":missing,tb.fresh?"ONLINE":missing,tc.fresh?"ONLINE":missing);
  lab_show(Checks,ChecksBuffer,CHECKS_SIZE,b,lab_text());
 }
 if(ticks>=120)continueStartup();
}
void ScreenSplashView::continueStartup(){if(leaving)return;leaving=true;application().gotoScreen1ScreenWipeTransitionEast();}
