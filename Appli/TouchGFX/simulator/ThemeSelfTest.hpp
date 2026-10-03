#ifndef THEMESELFTEST_HPP
#define THEMESELFTEST_HPP
#include "FingerSwipeSelfTest.hpp"
#include <gui/common/UiTheme.hpp>
// Optional simulator-only test: real touch samples exercise Designer callbacks.
class ThemeSelfTest : public FingerSwipeSelfTest {
public:
    ThemeSelfTest():enabled(getenv("PSU_THEME_SELFTEST")!=0),started(false),stage(0),frame(0),wait(0),log(0){if(enabled)log=fopen(getenv("PSU_THEME_SELFTEST"),"w");}
    virtual bool sampleTouch(int32_t& x,int32_t& y){
        if(!enabled)return FingerSwipeSelfTest::sampleTouch(x,y);
        FrontendApplication* app=static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
        if(!started){if(++wait>600)fail("startup timeout");if(app->swipePage()!=FrontendApplication::MAIN||app->isScreenTransitionActive())return false;psu_snapshot(&initial);app->gotoScreenSettingsScreenWipeTransitionWest();started=true;return false;}
        if(app->isScreenTransitionActive())return false;
        ++frame;
        const int positions[][2]={{400,400},{440,146},{440,318},{536,318},{632,318},{344,318},{344,146},{440,146},{728,32},{140,136},{728,32},{400,136},{728,32},{660,136},{728,32},{140,264},{728,32},{400,264},{728,32},{660,264},{728,32},{140,400},{728,32},{728,32}};
        x=positions[stage][0];y=positions[stage][1];
        if(frame==32){
            if(stage==1||stage==7)check(ui::Theme::dark(),"dark callback");
            if(stage==6)check(!ui::Theme::dark(),"light callback");
            if(stage>=2&&stage<=5)check(ui::Theme::accentIndex()==static_cast<unsigned>((stage-1)%4),"accent callback");
            if(stage>=8&&stage<=22&&stage%2==0)check(app->swipePage()==FrontendApplication::SETTINGS,"back navigation");
            if(stage==23)check(app->swipePage()==FrontendApplication::MAIN,"home navigation");
            PsuSnapshot now;psu_snapshot(&now);check(now.requested_mv==initial.requested_mv&&now.requested_ma==initial.requested_ma&&now.output_requested==initial.output_requested,"theme/navigation preserve PSU state");
            char name[50];snprintf(name,sizeof(name),"theme-%02d.bmp",stage);static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->saveScreenshot(const_cast<char*>("theme-selftest"),name);
            fprintf(log,"PASS stage %d\n",stage);fflush(log);
            if(++stage==24){fprintf(log,"PASS complete: palettes and seven settings pages\n");fclose(log);exit(0);}frame=0;return false;
        }
        return frame<=8;
    }
private:
    void fail(const char* text){if(log){fprintf(log,"FAIL stage %d: %s\n",stage,text);fclose(log);}exit(2);}
    void check(bool ok,const char* text){if(!ok)fail(text);}
    bool enabled,started;int stage,frame,wait;FILE* log;PsuSnapshot initial;
};
#endif
