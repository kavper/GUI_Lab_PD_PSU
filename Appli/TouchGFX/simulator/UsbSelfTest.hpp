#ifndef USBSELFTEST_HPP
#define USBSELFTEST_HPP
#include "ThemeSelfTest.hpp"
#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
// Opt-in simulator checks use the production ASCII parser and rendered widgets.
class UsbSelfTest : public ThemeSelfTest {
public:
    UsbSelfTest():enabled(getenv("PSU_USB_SELFTEST")!=0),started(false),stage(0),frame(0),wait(0),log(0){if(enabled)log=fopen(getenv("PSU_USB_SELFTEST"),"w");}
    virtual bool sampleTouch(int32_t& x,int32_t& y){
        if(!enabled)return ThemeSelfTest::sampleTouch(x,y);
        FrontendApplication* app=static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
        if(!started){if(++wait>600)fail("startup timeout");if(app->swipePage()!=FrontendApplication::MAIN||app->isScreenTransitionActive())return false;app->gotoScreenUsbPdScreenWipeTransitionWest();started=true;return false;}
        if(app->isScreenTransitionActive())return false;
        ScreenUsbPdView* view=static_cast<ScreenUsbPdView*>(app->getCurrentScreen());
        char line[256];const int current[]={320,-3000,0,8000,320,320,320,320};
        snprintf(line,sizeof(line),"TB bms=1 cfg=1 fault=0 sample=%d pack_mv=16020 i_pack_ma=%d\r\n",stage==4?0:1,current[stage]);
        g4_rx_bytes(psu_g4(),reinterpret_cast<const uint8_t*>(line),strlen(line),psu_app_now());
        snprintf(line,sizeof(line),"TC bq_ok=1 plug=1 pd_role=%d pd_mv=20000 pd_ma=3000 tps_vbus_mv=20010 bq_vbus_mv=19980 bq_iin_ma=600\r\n",stage==6?2:1);
        g4_rx_bytes(psu_g4(),reinterpret_cast<const uint8_t*>(line),strlen(line),psu_app_now());
        if(stage==5)psu_g4()->records[G4_RECORD_TB].ms=psu_app_now()-1601U;
        if(stage==7)ui::Theme::setDark(false);
        view->refresh();view->animateGauge();
        if(++frame==60){
            if(stage==0)check(view->gaugeValid&&view->gaugePosition>0,"charging to right");
            fprintf(log,"state valid=%d position=%d target=%d\n",view->gaugeValid,view->gaugePosition,view->gaugeTarget);
            if(stage==1)check(view->gaugeValid&&view->gaugePosition<0,"discharging to left");
            if(stage==2)check(view->gaugeValid&&view->gaugePosition==0,"zero centered");
            if(stage==3)check(view->gaugePosition==344,"overrange clamped");
            if(stage==4||stage==5)check(!view->gaugeValid&&!view->GaugeNeedle.isVisible(),"invalid/stale hidden");
            if(stage==6)check(view->Measured1Buffer[0]=='-'&&view->Measured1Buffer[1]=='-',"OTG input ADC not passed off as source measurement");
            char name[32];snprintf(name,sizeof(name),"usb-%d.bmp",stage);static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>("usb-selftest"),name);
            fprintf(log,"PASS stage %d\n",stage);fflush(log);
            if(++stage==8){fprintf(log,"PASS charging, discharging, zero, saturation, invalid, stale, OTG and light theme\n");fclose(log);exit(0);}frame=0;
        }
        return false;
    }
private:
    void fail(const char* s){if(log){fprintf(log,"FAIL %d: %s\n",stage,s);fclose(log);}exit(2);}
    void check(bool ok,const char* s){if(!ok)fail(s);}
    bool enabled,started;int stage,frame,wait;FILE* log;
};
#endif
