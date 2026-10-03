#ifndef FINGERSWIPESELFTEST_HPP
#define FINGERSWIPESELFTEST_HPP
#include <platform/driver/touch/SDL2TouchController.hpp>
#include <platform/hal/simulator/sdl2/HALSDL2.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/screen1_screen/Screen1View.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
#include "psu_app.h"
}
// Enabled only by PSU_FINGER_SELFTEST. Feeds the same TouchController interface
// used by the board, exercising HAL event generation and transition gating.
class FingerSwipeSelfTest : public touchgfx::SDL2TouchController
{
public:
    FingerSwipeSelfTest() : enabled(getenv("PSU_FINGER_SELFTEST")!=0),started(false),stage(0),frame(0),wait(0),log(0)
    {
        if(enabled) log=fopen(getenv("PSU_FINGER_SELFTEST"),"w");
    }
    virtual bool sampleTouch(int32_t& x,int32_t& y)
    {
        if(!enabled) return touchgfx::SDL2TouchController::sampleTouch(x,y);
        FrontendApplication* app=static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
        if(!started)
        {
            if(++wait>600) fail("main screen startup timeout");
            if(app->swipePage()!=FrontendApplication::MAIN || app->isScreenTransitionActive())return false;
            started=true;psu_snapshot(&initial);
        }
        ++frame;
        if(stage==4) // Enter 7 before testing cancellation restores the editor.
        {
            if(frame==25) static_cast<Screen1View*>(app->getCurrentScreen())->saveSwipeEditor(editor);
            if(frame>=35){stage=5;frame=0;return false;}
            x=562;y=275;return frame<=10;
        }
        const int directions[8]={1,-1,-1,1,0,1,-1,1};
        const FrontendApplication::SwipePage targets[8]={FrontendApplication::SETTINGS,FrontendApplication::MAIN,FrontendApplication::USB_PD,FrontendApplication::MAIN,FrontendApplication::MAIN,FrontendApplication::MAIN,FrontendApplication::USB_PD,FrontendApplication::MAIN};
        const int direction=directions[stage];
        const int distance=stage==5?100:300;
        x=stage==6?700:(direction>0?110:650);
        y=stage==6?28:220;
        if(frame>10)
        {
            int amount=frame>=40?distance:distance*(frame-10)/30;
            x+=direction*amount;
        }
        if(frame==50)
        {
            check(app->fingerSwipe().phase==FingerSwipeState::DRAGGING,"finger still controls transition while held");
            check(app->fingerSwipe().position==direction*distance,"screen displacement matches held finger");
            check(app->getCurrentScreen()->getRootContainer().getFirstChild()->getX()==direction*(distance-800),"incoming screen sits exactly beside outgoing snapshot");
            char name[64];snprintf(name,sizeof(name),"stage-%d-held.bmp",stage);
            const char* folder=getenv("PSU_FINGER_PREVIEWS");
            if(folder)static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>(folder),name);
        }
        if(frame==90)
        {
            check(app->swipePage()==targets[stage],"correct page after release/rollback");
            check(!app->isScreenTransitionActive(),"animation settled and accepts new input");
            check(app->getCurrentScreen()->getRootContainer().getFirstChild()->getX()==0,"settled view restored to zero offset");
            PsuSnapshot now;psu_snapshot(&now);
            check(now.requested_mv==initial.requested_mv && now.requested_ma==initial.requested_ma && now.output_requested==initial.output_requested,"swipes never apply setpoints or toggle output");
            if(stage==5)
            {
                MainSwipeEditorState restored;static_cast<Screen1View*>(app->getCurrentScreen())->saveSwipeEditor(restored);
                check(restored.target==editor.target && restored.length==editor.length && restored.replace==editor.replace && !memcmp(restored.text,editor.text,sizeof(editor.text)),"cancelled drag preserves unfinished editor input");
            }
            fprintf(log,"PASS stage %d\n",stage);fflush(log);
            if(++stage==8){fprintf(log,"PASS complete: four routes, held finger tracking, cancellation, editor restoration and output-button cancellation\n");fclose(log);exit(0);}
            frame=0;return false;
        }
        return frame<=65;
    }
private:
    void fail(const char* text){if(log){fprintf(log,"FAIL stage %d frame %d: %s\n",stage,frame,text);fclose(log);}exit(2);}
    void check(bool ok,const char* text){if(!ok)fail(text);}
    bool enabled,started;
    int stage,frame,wait;
    FILE* log;
    PsuSnapshot initial;
    MainSwipeEditorState editor;
};
#endif
