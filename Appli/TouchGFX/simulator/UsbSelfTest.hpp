#ifndef USBSELFTEST_HPP
#define USBSELFTEST_HPP
#include "ThemeSelfTest.hpp"
#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/screen1_screen/Screen1View.hpp>
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
            if(++stage==8){fprintf(log,"PASS press selection, charging, red discharging, zero, dynamic 250 W range, invalid, stale, OTG and light theme\n");fclose(log);exit(0);}frame=0;
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
