#ifndef USBSELFTEST_HPP
#define USBSELFTEST_HPP
#include "ThemeSelfTest.hpp"
#include <texts/TextKeysAndLanguages.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screensequencer_screen/ScreenSequencerView.hpp>
#include <gui/screenextcharger_screen/ScreenExtChargerView.hpp>
// Opt-in simulator checks use the production binary parser and rendered widgets.
class UsbSelfTest : public ThemeSelfTest {
public:
    UsbSelfTest():enabled(getenv("PSU_USB_SELFTEST")!=0),started(false),stage(0),frame(0),wait(0),log(0){if(enabled)log=fopen(getenv("PSU_USB_SELFTEST"),"w");}
    virtual bool sampleTouch(int32_t& x,int32_t& y){
        if(!enabled)return ThemeSelfTest::sampleTouch(x,y);
        FrontendApplication* app=static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
        if(!started){
            if(++wait>600)fail("startup timeout");
            if(app->swipePage()!=FrontendApplication::MAIN||app->isScreenTransitionActive())return false;
            Screen1View* main=static_cast<Screen1View*>(app->getCurrentScreen());
            main->handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::PRESSED,350,310));
            check(main->editTarget==Screen1View::EDIT_CURRENT && main->CurrentSelection.isVisible(),"current selects on PRESSED before release");
            main->key1();const uint8_t length=main->editLength;
            main->handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::RELEASED,350,310));
            check(main->editLength==length,"release does not repeat selection and reset editor");
            main->handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::PRESSED,350,150));
            check(main->editTarget==Screen1View::EDIT_VOLTAGE && main->VoltageSelection.isVisible(),"voltage selects on PRESSED before release");
            main->handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::CANCEL,350,150));
            fprintf(log,"PASS immediate voltage/current press and release preserves edit\n");
            app->gotoScreenUsbPdScreenWipeTransitionWest();started=true;return false;
        }
        if(app->isScreenTransitionActive())return false;
        if(stage==8){app->gotoScreen1ScreenWipeTransitionEast();stage=9;frame=0;return false;}
        if(stage==9){
            Screen1View* main=static_cast<Screen1View*>(app->getCurrentScreen());
            uint8_t p[72]={0};p[0]=p[1]=p[52]=1;w16(p+44,16000);w32(p+48,static_cast<uint32_t>(-15625));emit(0x11,p,72);
            memset(p,0,sizeof(p));p[0]=1;p[1]=0x85;p[7]=p[10]=1;p[8]=1;w32(p+36,600);w32(p+52,20010);emit(0x12,p,64);
            psu_g4()->records[G4_RECORD_T].valid=1;psu_g4()->records[G4_RECORD_T].ms=psu_app_now();
            main->setHostAuxMetrics();
            if(frame==10){main->BatteryValue.invalidate();touchgfx::Unicode::snprintf(main->BatteryValueBuffer,main->BATTERYVALUE_SIZE,"+5.0 W");main->BatteryValue.invalidate();}
            if(++frame<30)return false;
            check(main->BatteryValueBuffer[0]=='-' && main->BatteryValue.getColor()==ui::Theme::color(ui::NEGATIVE),"main screen has red net battery discharge");
            check(main->BatteryValue.getY()==32 && main->PowerLabel.getTypedText().getId()==T_TXT_HEADER_VPREREG,"battery power and correctly named prereg in header");
            static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>("usb-selftest"),const_cast<char*>("main-power.bmp"));
            app->gotoScreenSequencerScreenWipeTransitionWest();stage=10;frame=0;return false;
        }
        if(stage==10){
            ScreenSequencerView* v=static_cast<ScreenSequencerView*>(app->getCurrentScreen());
            if(frame==0){
              v->seqCycleMore();v->seqCycleMore();check(psu_sequencer()->loops_requested==3&&psu_sequencer()->mode==PSU_SEQ_N,"three cycles GUI");
              v->seqCycles();v->seqKey1();v->seqKey2();v->seqKeyApply();check(psu_sequencer()->loops_requested==12,"cycle count keypad stores integer");
              v->seqInfinity();check(psu_sequencer()->mode==PSU_SEQ_INFINITE,"continuous GUI");
            }
            v->refresh();
            if(++frame<30)return false;
            check(v->CycleInfinityButton.isVisible(),"cycle controls visible without step keypad");
            static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>("usb-selftest"),const_cast<char*>("sequence-cycles.bmp"));
            v->seqFieldVolt();ui::Theme::setDark(true);stage=11;frame=0;return false;
        }
        if(stage==11||stage==12){
          ScreenSequencerView* v=static_cast<ScreenSequencerView*>(app->getCurrentScreen());
          ui::ThemeScreen::get().sync();
          if(++frame<40)return false;
          check(v->SeqKey1.getParent()==&v->SeqKeypad && v->SeqKeypad.isVisible(),"keypad and themed surfaces share nested container");
          static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>("usb-selftest"),const_cast<char*>(stage==11?"sequence-keypad-dark.bmp":"sequence-keypad-light.bmp"));
          if(stage==11){ui::Theme::setDark(false);stage=12;frame=0;return false;}
          app->gotoScreenExtChargerScreenWipeTransitionWest();stage=13;frame=0;return false;
        }
        if(stage==13){
          ScreenExtChargerView* v=static_cast<ScreenExtChargerView*>(app->getCurrentScreen());
          if(frame==0){v->chooseChem(0);v->field0();v->chargeKey4();v->chargeKeyApply();v->field4();v->chargeKey1();v->chargeKey6();v->chargeKeyDot();v->chargeKey4();v->chargeKeyApply();check(psu_charger()->profile.cells==4 && psu_chg_target_mv(&psu_charger()->profile)==16400,"4S 16.4 V target through actual GUI keypad");}
          if(frame==0){v->field1();v->chargeKey1();v->chargeKeyApply();check(psu_charger()->profile.cc_ma==1000,"explicit 1 A through GUI without capacity ceiling");v->field2();v->chargeKey0();v->chargeKeyDot();v->chargeKey1();v->chargeKeyApply();check(psu_charger()->profile.cc_ma==1000&&psu_charger()->profile.term_ma==100,"end threshold never changes charge current");}
          v->refresh();if(++frame<30)return false;
          static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>("usb-selftest"),const_cast<char*>("charger-target.bmp"));
          v->allOff();PsuSnapshot snap;psu_snapshot(&snap);check(snap.power_shutdown_requested&&!snap.output_requested,"header requests complete power shutdown");
          fprintf(log,"PASS header BAT PWR/VPREREG; dark/light nested sequence keypad; 4S 16.4 V target GUI; header shutdown\n");fclose(log);exit(0);
        }
        ScreenUsbPdView* view=static_cast<ScreenUsbPdView*>(app->getCurrentScreen());
        const int current[]={320,-3000,0,-15625,320,320,320,-3000};
        uint8_t payload[72]={0};payload[0]=payload[1]=1;payload[52]=stage==4?0:1;
        w16(payload+44,16000);w32(payload+48,static_cast<uint32_t>(current[stage]));emit(0x11,payload,72);
        memset(payload,0,sizeof(payload));payload[0]=1;payload[1]=0x85;payload[7]=payload[10]=stage==6?2:1;payload[8]=1;payload[9]=0x42;
        w32(payload+36,600);w32(payload+32,19980);w32(payload+52,20010);w32(payload+56,20000);w32(payload+60,3000);
        emit(0x12,payload,64);
        if(stage==5)psu_g4()->records[G4_RECORD_TB].ms=psu_app_now()-1601U;
        if(stage==7)ui::Theme::setDark(false);
        view->refresh();view->animateGauge();
        if(++frame==60){
            if(stage==0){
                check(view->gaugeValid&&view->gaugePosition>0,"charging to right");
                check(view->Measured1Buffer[0]=='0' && view->Measured2Buffer[0]=='1',"SINK current/power ADC visible in fixture");
            }
            fprintf(log,"state valid=%d position=%d target=%d\n",view->gaugeValid,view->gaugePosition,view->gaugeTarget);
            if(stage==1||stage==7) {
                check(view->gaugeValid&&view->gaugePosition<0,"discharging to left");
                check(view->GaugeDischarge.getColor()==ui::Theme::color(ui::NEGATIVE) && view->DischargeLabel.getColor()==ui::Theme::color(ui::NEGATIVE),"negative track and caption red in both themes");
            }
            if(stage==2)check(view->gaugeValid&&view->gaugePosition==0,"zero centered");
            if(stage==3)check(view->gaugeScale.range()==300000 && view->gaugePosition==-286,"minus 250 W expands to symmetric 300 W without saturation");
            if(stage==4||stage==5)check(!view->gaugeValid&&!view->GaugeNeedle.isVisible(),"invalid/stale hidden");
            if(stage==6)check(view->Measured1Buffer[0]=='-'&&view->Measured1Buffer[1]=='-',"OTG input ADC not passed off as source measurement");
            char name[32];snprintf(name,sizeof(name),"usb-%d.bmp",stage);static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>("usb-selftest"),name);
            fprintf(log,"PASS stage %d\n",stage);fflush(log);
            if(++stage==8){fprintf(log,"PASS press selection, charging, red discharging, zero, dynamic 250 W range, invalid, stale, OTG and light theme\n");}frame=0;
        }
        return false;
    }
private:
    static void w16(uint8_t* p,uint16_t v){p[0]=v;p[1]=v>>8;}
    static void w32(uint8_t* p,uint32_t v){w16(p,v);w16(p+2,v>>16);}
    void emit(uint8_t type,const uint8_t* p,size_t n){uint8_t f[120];size_t len=g4_frame(f,type,static_cast<uint8_t>(frame),p,n);g4_rx_bytes(psu_g4(),f,len,psu_app_now());}
    void fail(const char* s){if(log){fprintf(log,"FAIL %d: %s\n",stage,s);fclose(log);}exit(2);}
    void check(bool ok,const char* s){if(!ok)fail(s);}
    bool enabled,started;int stage,frame,wait;FILE* log;
};
#endif
