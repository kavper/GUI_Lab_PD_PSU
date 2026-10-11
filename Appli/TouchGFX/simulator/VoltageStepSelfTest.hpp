#pragma once
#include "UsbSelfTest.hpp"
#include <cstdio>
#include <cstdlib>

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
        const int buttons[]={450,400,450,400,350,400,350};
        const int offsets[]={100,100,1100,1100,1090,1090,990};
        x=buttons[stage];y=201;
        if(frame==30){
            PsuSnapshot current;psu_snapshot(&current);
            if(current.requested_mv!=initial.requested_mv+offsets[stage])fail("voltage step/touch mismatch");
            if(current.requested_ma!=initial.requested_ma||current.output_requested!=initial.output_requested)fail("current/output changed");
            fprintf(log,"PASS stage %d: %lu mV\n",stage,(unsigned long)current.requested_mv);fflush(log);
            if(++stage==7){fprintf(log,"PASS: touch +/- and 10 mV / 100 mV / 1 V cycle; current/output unchanged\n");fclose(log);exit(0);}
            frame=0;return false;
        }
        return frame<=8;
    }
private:
    bool enabled,started;int frame,stage;FILE* log;PsuSnapshot initial;
    void fail(const char* reason){if(log){fprintf(log,"FAIL: %s\n",reason);fclose(log);}exit(2);}
};
