#include <gui/screensplash_screen/ScreenSplashView.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_version.h"
}
void ScreenSplashView::setupScreen(){ScreenSplashViewBase::setupScreen();char b[40];snprintf(b,sizeof(b),"FW %s / %s",PSU_FW_VERSION,PSU_FW_DATE);lab_show(SplashBuild,SplashBuildBuffer,SPLASHBUILD_SIZE,b,lab_muted());handleTickEvent();}
void ScreenSplashView::handleTickEvent(){
 ++ticks;if(ticks%10!=1)return;TelemetryRecord t(0),tb(1),tc(2);char b[256];
 snprintf(b,sizeof(b),"HMI interface                 READY\nG4 / PSU telemetry            %s\nBMS / battery telemetry       %s\nUSB-C / charger telemetry     %s",t.fresh?"RECEIVED":ticks>240?"NO RESPONSE":"WAITING",tb.fresh?"RECEIVED":ticks>240?"NO RESPONSE":"WAITING",tc.fresh?"RECEIVED":ticks>240?"NO RESPONSE":"WAITING");
 lab_show(Checks,ChecksBuffer,CHECKS_SIZE,b,touchgfx::Color::getColorFromRGB(230,237,250));
 if(ticks>180&&t.fresh&&tb.fresh&&tc.fresh)continueStartup();
}
void ScreenSplashView::continueStartup(){application().gotoScreen1ScreenWipeTransitionEast();}
