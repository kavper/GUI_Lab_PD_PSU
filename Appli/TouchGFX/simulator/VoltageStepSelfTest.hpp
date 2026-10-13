#pragma once
#include "UsbSelfTest.hpp"
#include <cstdio>
#include <cstdlib>
#include <gui/common/UiTheme.hpp>

// Opt-in simulator check using the same touch samples as the target.
class VoltageStepSelfTest : public UsbSelfTest {
public:
    VoltageStepSelfTest():enabled(getenv("PSU_VSTEP_SELFTEST")!=0),started(false),frame(0),stage(0),log(0) {
        if(enabled)log=fopen(getenv("PSU_VSTEP_SELFTEST"),"w");
    }
    virtual bool sampleTouch(int32_t& x,int32_t& y) {
        if(!enabled)return UsbSelfTest::sampleTouch(x,y);
        FrontendApplication* app=static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
        if(!started){
            if(++frame>600)fail("startup timeout");
            if(app->swipePage()!=FrontendApplication::MAIN||app->isScreenTransitionActive())return false;
            psu_snapshot(&initial);started=true;frame=0;
        }
        ++frame;
        if(stage==14){if(frame==10){screenshot("steps-light.bmp");fclose(log);exit(0);}return false;}
        if(stage==0&&frame==1)screenshot("steps-dark.bmp");
        const int buttons[]={450,400,450,400,350,400,350};
        const int offsets[]={100,100,1100,1100,1090,1090,990};
        x=buttons[stage%7];y=stage<7?201:361;
        if(frame==30){
            PsuSnapshot current;psu_snapshot(&current);
            const unsigned voltageOffset=stage<7?offsets[stage]:990;
            const unsigned currentOffset=stage<7?0:offsets[stage-7];
            if(current.requested_mv!=initial.requested_mv+voltageOffset)fail("voltage step/touch mismatch");
            if(current.requested_ma!=initial.requested_ma+currentOffset)fail("current step/touch mismatch");
            if(current.output_requested!=initial.output_requested)fail("output changed");
            fprintf(log,"PASS stage %d: %lu mV / %lu mA\n",stage,(unsigned long)current.requested_mv,(unsigned long)current.requested_ma);fflush(log);
            if(++stage==14){fprintf(log,"PASS: voltage/current +/- and three independent steps; output unchanged\n");ui::Theme::setDark(false);}
            frame=0;return false;
        }
        return frame<=8;
    }
private:
    bool enabled,started;int frame,stage;FILE* log;PsuSnapshot initial;
    void screenshot(const char* name){const char* folder=getenv("PSU_STEP_PREVIEWS");if(folder)static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>(folder),const_cast<char*>(name));}
    void fail(const char* reason){if(log){fprintf(log,"FAIL: %s\n",reason);fclose(log);}exit(2);}
};
