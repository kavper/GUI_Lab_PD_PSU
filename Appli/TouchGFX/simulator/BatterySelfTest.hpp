#ifndef BATTERYSELFTEST_HPP
#define BATTERYSELFTEST_HPP
#include "VoltageStepSelfTest.hpp"
#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
class BatterySelfTest:public VoltageStepSelfTest {
public:
 BatterySelfTest():enabled(getenv("PSU_BATTERY_SELFTEST")!=0),started(false),stage(0),frame(0),wait(0),log(0){if(enabled)log=fopen(getenv("PSU_BATTERY_SELFTEST"),"w");}
 virtual bool sampleTouch(int32_t& x,int32_t& y){
  if(!enabled)return VoltageStepSelfTest::sampleTouch(x,y);
  auto* app=static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
  if(!started){if(++wait>600)fail("startup timeout");if(app->swipePage()!=FrontendApplication::MAIN||app->isScreenTransitionActive())return false;app->gotoScreenBatteryScreenWipeTransitionWest();started=true;return false;}
  if(app->isScreenTransitionActive())return false;
  ++frame;
  x=stage==5?496:stage==6?694:stage==7?104:stage==8?296:stage==9?728:694;
  y=stage==9?32:stage==10?449:97;
  if(stage<8){
   auto* v=static_cast<ScreenBatteryView*>(app->getCurrentScreen());
   uint8_t p[72]={0};p[0]=p[1]=1;p[52]=stage==2?0:1;
   const uint16_t cells[]={4050,4060,4070,65535,4090};
   for(unsigned i=0;i<5;i++)w16(p+26+2*i,stage==4?4050:cells[i]);
   w16(p+44,16320);w32(p+48,stage==1?320:(uint32_t)-1500);p[70]=stage==4?0:2;p[71]=1;w16(p+64,750);
   emit(0x11,p,72);if(stage==3)psu_g4()->records[G4_RECORD_TB].ms=psu_app_now()-1501;
   v->refresh();
   if(frame==32){
    if(stage==0){check(v->CellValue3Buffer[0]=='U',"physical CH4 unused");check(v->CellValue4Buffer[0]=='4',"physical CH5 present");check(v->CellStatus1Buffer[0]=='B',"balance bit maps to physical CH2");check(v->CellStatus0Buffer[0]=='L'&&v->CellStatus4Buffer[0]=='H',"physical lowest/highest");check(v->FlowStateBuffer[0]=='D'&&v->PackPowerBuffer[0]=='2',"discharge / actual pack power");}
    if(stage==1)check(v->FlowStateBuffer[0]=='C',"charging state");
    if(stage==2||stage==3)check(v->PackPowerBuffer[0]=='-'&&v->CellValue0Buffer[0]=='-',"invalid/stale never shown as valid measurement");
    if(stage==4)check(v->CellStatus0Buffer[0]=='M',"equal cells min/max");
    if(stage==5)check(v->page==1&&v->ChargerPanel.isVisible()&&!v->CellCard0.isVisible(),"charger tab actual touch");
    if(stage==6)check(v->page==2&&v->trend.isVisible()&&!v->ChargerPanel.isVisible(),"energy tab actual touch");
    if(stage==7)check(v->page==0&&v->CellCard4.isVisible()&&!v->trend.isVisible(),"battery tab restores all five channels");
   }
  }
  if(frame==32){
   char name[40];snprintf(name,sizeof(name),"battery-%d.bmp",stage);static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>("battery-selftest"),name);
   if(stage==9){check(app->swipePage()==FrontendApplication::SETTINGS,"BMS back to menu");app->gotoScreenBatteryScreenWipeTransitionWest();}
   if(stage==10)check(app->swipePage()==FrontendApplication::USB_PD,"USB-C quick link");
   fprintf(log,"PASS stage %u\n",stage);fflush(log);
   if(++stage==11){fprintf(log,"PASS physical five channels, balance, unused, min/max, invalid/stale, battery/charger/energy tabs and BMS/USB navigation\n");fclose(log);exit(0);}frame=0;return false;
  }
  return stage>=5&&frame<=8;
 }
private:
 static void w16(uint8_t* p,uint16_t v){p[0]=v;p[1]=v>>8;}
 static void w32(uint8_t* p,uint32_t v){w16(p,v);w16(p+2,v>>16);}
 void emit(uint8_t type,const uint8_t* p,size_t n){uint8_t b[120];size_t len=g4_frame(b,type,frame,p,n);g4_rx_bytes(psu_g4(),b,len,psu_app_now());}
 void check(bool ok,const char* msg){if(!ok)fail(msg);}
 void fail(const char* msg){if(log){fprintf(log,"FAIL %u: %s\n",stage,msg);fclose(log);}exit(2);}
 bool enabled,started;unsigned stage,frame,wait;FILE* log;
};
#endif
