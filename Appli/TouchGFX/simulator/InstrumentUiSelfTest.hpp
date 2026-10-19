#pragma once
#include "SettingsSelfTest.hpp"
#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
#include <gui/screenextcharger_screen/ScreenExtChargerView.hpp>
class InstrumentUiSelfTest:public SettingsSelfTest {
public:
 InstrumentUiSelfTest():log(0),enabled(getenv("PSU_INSTRUMENT_SELFTEST")!=0),started(false),stage(0),frame(0),boot(0),parent(false){if(enabled)log=fopen(getenv("PSU_INSTRUMENT_SELFTEST"),"w");}
 virtual bool sampleTouch(int32_t& x,int32_t& y){
  if(!enabled)return SettingsSelfTest::sampleTouch(x,y);
  auto *app=static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
  if(!started){
   if(++boot==20)shot("00-startup.bmp");
   if(boot>600)fail("startup timeout");
   if(app->swipePage()!=FrontendApplication::MAIN||app->isScreenTransitionActive())return false;
   shot("01-main.bmp");started=true;open(app);return false;
  }
  if(app->isScreenTransitionActive())return false;
  if(parent){parent=false;app->gotoScreenBmsScreenWipeTransitionWest();return false;}
  ++frame;
  if(frame==24){
   if(app->swipePage()!=routes()[stage].expected)fail("wrong screen");
   shot(routes()[stage].name);fprintf(log,"PASS view %s\n",routes()[stage].name);fflush(log);
   if(stage==1){auto *v=static_cast<ScreenUsbPdView*>(app->getCurrentScreen());G4PdCapabilities c;g4_pd_capabilities_snapshot(psu_g4(),&c);if(c.count!=7||!v->PdoSelected.isVisible())fail("full source PDO list / selected RDO");}
   if(stage==2){auto *v=static_cast<ScreenBatteryView*>(app->getCurrentScreen());if(!v->CellCard4.isVisible()||!v->ChargerPanel.isVisible()||!v->EnergyPanel.isVisible()||!v->trend.isVisible())fail("battery overview hides useful information");if(v->CellValue3Buffer[0]!='U')fail("physical CH4 unused");}
  }
  if(stage==1 && frame==28){
   auto *v=static_cast<ScreenUsbPdView*>(app->getCurrentScreen());G4Port *port=psu_g4();const G4PdCapabilities saved=port->pd_caps;
   port->pd_caps.ms=psu_app_now()-1001;v->refresh();if(v->PdoSelected.isVisible())fail("stale PDO list selected");
   port->pd_caps=saved;++port->pd_caps.attach_generation;v->refresh();if(v->PdoSelected.isVisible())fail("previous cable session selected");
   port->pd_caps=saved;v->refresh();if(!v->PdoSelected.isVisible())fail("restored current PDO selection");
   fprintf(log,"PASS stale and previous-session PDOs hidden\n");
  }
  if(stage==7 && frame==28){
   auto *v=static_cast<ScreenExtChargerView*>(app->getCurrentScreen());
   const uint32_t original=psu_charger()->profile.cc_ma;
   tap(v,370,190);
   if(!v->editing||!v->ChargeKey1.isVisible()||v->SessionCard.isVisible())fail("charger editor touch / keypad visibility");
   tap(v,560,160);tap(v,688,425);
   if(v->editing||psu_charger()->profile.cc_ma!=1000||!v->SessionCard.isVisible())fail("charger keypad APPLY actual touch");
   tap(v,370,190);tap(v,644,160);
   app->handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::PRESSED,20,330));
   app->handleDragEvent(touchgfx::DragEvent(touchgfx::DragEvent::DRAGGED,20,330,60,330));
   app->handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::RELEASED,60,330));
   uint32_t draft=0;
   if(!v->editing||!psu_editor_parse_milli(&v->editor,&draft)||draft!=2000)fail("cancelled edge swipe preserves charger draft");
   tap(v,370,190);
   if(v->editing||psu_charger()->profile.cc_ma!=1000)fail("charger cancel must not save draft");
   psu_charger()->profile.cc_ma=original;v->refresh();
   fprintf(log,"PASS charger touch edit, APPLY and draft cancellation\n");
  }
  // Main/menu keep the existing interactive swipe. Other pages use a safe edge gesture.
  if(stage==0&&frame==36){next(app);return false;}
  x=20;y=270;
  if(frame>=40&&frame<44)return true;
  if(frame>=44&&frame<52){x=20+(frame-43)*20;return true;}
  if(frame==64){
   const auto expected=stage==3?FrontendApplication::BATTERY:FrontendApplication::SETTINGS;
   if(app->swipePage()!=expected)fail("edge swipe back route");
   fprintf(log,"PASS edge back %s\n",routes()[stage].name);fflush(log);next(app);
  }
  return false;
 }
private:
 struct Route {const char *name;void(FrontendApplication::*open)();FrontendApplication::SwipePage expected;};
 static const Route *routes(){
  static const Route r[]={
   {"02-menu.bmp",&FrontendApplication::gotoScreenSettingsScreenWipeTransitionWest,FrontendApplication::SETTINGS},
   {"03-usb-pd.bmp",&FrontendApplication::gotoScreenUsbPdScreenWipeTransitionWest,FrontendApplication::USB_PD},
   {"04-battery.bmp",&FrontendApplication::gotoScreenBatteryScreenWipeTransitionWest,FrontendApplication::BATTERY},
   {"05-bms-details.bmp",&FrontendApplication::gotoScreenBmsScreenWipeTransitionWest,FrontendApplication::BMS},
   {"06-output.bmp",&FrontendApplication::gotoScreenMeasurementsScreenWipeTransitionWest,FrontendApplication::DETAIL},
   {"07-presets.bmp",&FrontendApplication::gotoScreenPresetsScreenWipeTransitionWest,FrontendApplication::DETAIL},
   {"08-sequencer.bmp",&FrontendApplication::gotoScreenSequencerScreenWipeTransitionWest,FrontendApplication::DETAIL},
   {"09-charger.bmp",&FrontendApplication::gotoScreenExtChargerScreenWipeTransitionWest,FrontendApplication::DETAIL},
   {"10-diagnostics.bmp",&FrontendApplication::gotoScreenDiagnosticsScreenWipeTransitionWest,FrontendApplication::DETAIL},
   {"11-network.bmp",&FrontendApplication::gotoScreenNetworkScreenWipeTransitionWest,FrontendApplication::DETAIL},
   {"12-appearance.bmp",&FrontendApplication::gotoScreenAppearanceScreenWipeTransitionWest,FrontendApplication::DETAIL},
   {"13-power-path.bmp",&FrontendApplication::gotoScreenPowerPathScreenWipeTransitionWest,FrontendApplication::DETAIL},
   {"14-protection.bmp",&FrontendApplication::gotoScreenProtectionScreenWipeTransitionWest,FrontendApplication::DETAIL},
   {"15-service.bmp",&FrontendApplication::gotoScreenServiceScreenWipeTransitionWest,FrontendApplication::DETAIL}};
  return r;
 }
 void open(FrontendApplication *app){frame=0;if(stage==3){parent=true;app->gotoScreenBatteryScreenWipeTransitionWest();}else(app->*routes()[stage].open)();}
 void next(FrontendApplication *app){if(++stage==14){fprintf(log,"PASS complete: all screens and edge returns\n");fclose(log);exit(0);}open(app);}
 void tap(ScreenExtChargerView *v,int x,int y){v->handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::PRESSED,x,y));v->handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::RELEASED,x,y));}
 void shot(const char *name){
  static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>("instrument-refine"),const_cast<char*>(name));
  char path[200];snprintf(path,sizeof(path),"screenshots/instrument-refine/%s.ppm",name);FILE *f=fopen(path,"wb");
  if(!f){fail("framebuffer screenshot");}fprintf(f,"P6\n800 480\n255\n");
  const uint16_t *pixels=touchgfx::HAL::getInstance()->getTFTFrameBuffer();
  for(unsigned i=0;i<800*480;i++){uint16_t v=pixels[i];unsigned char rgb[]={static_cast<unsigned char>((v>>11)*255/31),static_cast<unsigned char>(((v>>5)&63)*255/63),static_cast<unsigned char>((v&31)*255/31)};fwrite(rgb,1,3,f);}fclose(f);
 }
 void fail(const char *why){if(log){fprintf(log,"FAIL stage %u frame %u: %s\n",stage,frame,why);fclose(log);}exit(2);}
 FILE *log;bool enabled,started;unsigned stage,frame,boot;bool parent;
};
