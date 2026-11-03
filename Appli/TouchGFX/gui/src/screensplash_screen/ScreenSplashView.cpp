#include <gui/common/KitLayout.hpp>
#include <gui/screensplash_screen/ScreenSplashView.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/FrontendApplication.hpp>
extern "C" {
#include "psu_version.h"
}
void ScreenSplashView::setupScreen(){
 ticks=0;leaving=false;ScreenSplashViewBase::setupScreen();
 static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::SPLASH);
 layout();
 auto& theme=ui::ThemeScreen::get();theme.begin(*this);
 theme.box(SplashBackground,ui::BACKGROUND);theme.box(StartupTrack,ui::RAISED);
 theme.text(SplashTitle,ui::TEXT);theme.text(SplashSubtitle,ui::MUTED);
 theme.text(SplashThesis,ui::MUTED);theme.text(SplashBuild,ui::MUTED);
 theme.text(StartupHint,ui::MUTED);
 theme.button(ContinueButton,ui::PRIMARY);theme.apply();
 char b[80];snprintf(b,sizeof(b),"FW %s \xC2\xB7 %s",PSU_FW_VERSION,PSU_FW_DATE);build1.set(b);
 build2.set(PSU_FW_BUILD);
 kit::applyDesignerLayout(FrontendApplication::SPLASH,getRootContainer());handleTickEvent();
}
void ScreenSplashView::layout(){
 using kit::place;
 kit::clearNoSwipe();
 kit::hide(Checks);
 side.setup(0,0,300,480,ui::SURFACE); sideEdge.setup(300,0,1,480,ui::BORDER);
 insert(&SplashBackground,sideEdge); insert(&SplashBackground,side);
 place(AghLogo,32,36,86,159);
 kit::hide(SplashTitle); kit::hide(SplashSubtitle); kit::hide(SplashThesis); kit::hide(SplashBuild); kit::hide(StartupHint);
 title.setup(kit::HEAD,kit::LEFT,32,214,256,30,ui::TEXT); title.set("Lab PD PSU");
 subtitle.setup(kit::BODY,kit::LEFT,32,250,256,20,ui::MUTED); subtitle.set("Laboratory power supply");
 specs.setup(kit::BODY,kit::LEFT,32,270,256,20,ui::MUTED); specs.set("USB-PD \xC2\xB7 battery \xC2\xB7 0\xE2\x80\x93" "27 V / 5 A");
 build1.setup(kit::M12,kit::LEFT,32,424,260,18,ui::MUTED); build2.setup(kit::M12,kit::LEFT,32,442,260,18,ui::MUTED);
 hint.setup(kit::SUB,kit::LEFT,336,400,240,34,ui::MUTED); hint.set("Missing modules can connect later \xE2\x80\x93\nyou can continue offline.");
 add(title); add(subtitle); add(specs); add(build1); add(build2); add(hint);

 checkCaption.setup(kit::CAP,kit::LEFT,336,40,200,16,ui::MUTED); checkCaption.set("SYSTEM CHECK");
 lockNote.setup(kit::SUB,kit::RIGHT,560,40,206,16,ui::MUTED); lockNote.set("Output stays OFF");
 add(checkCaption); add(lockNote);
 const char* names[ROWS]={"Supervisor G4 · output control","LDO stage G0","Battery / BMS","USB-C power delivery","Ethernet"};
 for(int i=0;i<ROWS;i++){
  const int y=66+i*44;
  rowMark[i].setup(332,y+8,28,26,"",kit::Btn::GHOST,kit::BODYM); rowMark[i].setBase(ui::BACKGROUND); rowMark[i].setTouchable(false); rowMark[i].setIconSize(18);
  rowName[i].setup(kit::BODY,kit::LEFT,366,y+10,260,22,ui::TEXT); rowName[i].set(names[i]);
  rowState[i].setup(kit::M14,kit::RIGHT,620,y+10,146,22,ui::MUTED);
  rowLine[i].setup(336,y+43,430,1,ui::BORDER);
  add(rowMark[i]); add(rowName[i]); add(rowState[i]); add(rowLine[i]);
 }
 place(StartupTrack,336,372,430,6); place(StartupProgress,336,372,1,6);
 place(ContinueButton,596,402,170,52);
}
void ScreenSplashView::refreshChecks(){
 TelemetryRecord t(0),tb(1),tc(2);PsuSnapshot s;psu_snapshot(&s);
 const bool late=ticks>80;
 const bool ok[ROWS]={t.fresh,s.g0_connected&&!s.g0_stale,tb.fresh,tc.fresh,s.net_state==PSU_NET_BOUND};
 int64_t soc=0,series=0,plug=0,role=0;
 TelemetryRecord gauge(G4_RECORD_BATT);
 const bool haveSoc=gauge.get("soc_permille",soc)||tb.get("soc_permille",soc);
 static char batName[40],batState[16];
 if(tb.get("series",series)&&series>0)snprintf(batName,sizeof(batName),"Battery / BMS \xC2\xB7 %ldS LFP",(long)series);else snprintf(batName,sizeof(batName),"Battery / BMS");
 rowName[2].set(batName);
 const bool usbWait=tc.fresh&&tc.get("plug",plug)&&plug&&!(tc.get("pd_role",role)&&(role==1||role==2));
 for(int i=0;i<ROWS;i++){
  const bool optional=i==4;
  const char* state=ok[i]?"ONLINE":optional?(s.ethernet_configured?"WAITING":"NOT FITTED"):late?"NOT CONNECTED":"CONNECTING";
  if(i==2&&ok[i]&&haveSoc){snprintf(batState,sizeof(batState),"%ld %%",(long)((soc+5)/10));state=batState;}
  if(i==3&&usbWait)state="NEGOTIATING";
  if(i==4&&ok[i]){static char ip[20];snprintf(ip,sizeof(ip),"%u.%u.%u.%u",s.ip[0],s.ip[1],s.ip[2],s.ip[3]);state=ip;}
  const ui::Role role=ok[i]?ui::POSITIVE:(late&&!optional)?ui::CAUTION:ui::MUTED;
  rowState[i].set(state,role);
  rowMark[i].setIcon(ok[i]?kit::ICON_CHECK:kit::ICON_NONE); rowMark[i].setLabel(ok[i]?"":late&&!optional?"!":"\xC2\xB7"); rowMark[i].setInk(role);
 }
}
void ScreenSplashView::handleTickEvent(){
 if(leaving)return;
 ++ticks;
 if(ticks%3==1){StartupProgress.invalidate();StartupProgress.setWidth(ticks>=120?430:ticks*430/120);StartupProgress.setColor(ui::Theme::color(ui::ACCENT));StartupProgress.invalidate();}
 if(ticks%12==1)refreshChecks();
 if(ticks>=120)continueStartup();
}
void ScreenSplashView::continueStartup(){if(leaving)return;leaving=true;static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->go(FrontendApplication::MAIN,FrontendApplication::FORWARD);}
