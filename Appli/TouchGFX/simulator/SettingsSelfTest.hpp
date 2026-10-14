#pragma once
#include "BatterySelfTest.hpp"
#include <gui/common/SettingsPersistence.hpp>
class SettingsSelfTest : public BatterySelfTest {
public:
    SettingsSelfTest():mode(getenv("PSU_SETTINGS_SELFTEST")),frames(0),changed(false){}
    virtual bool sampleTouch(int32_t& x,int32_t& y) {
        if(!mode)return BatterySelfTest::sampleTouch(x,y);
        FrontendApplication *app=static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
        if(app->swipePage()!=FrontendApplication::MAIN || app->isScreenTransitionActive())return false;
        ++frames;
        if(frames==10) {
            if(!strcmp(mode,"save")) {
                ui::Theme::setDark(false);ui::Theme::setAccent(2);
                ui::Sound::setVolume(70);
                while(ui::Sound::tone()!=1)ui::Sound::cycleTone();
                if(ui::Sound::enabled())ui::Sound::toggle();
                if(!psu_preset_set(0,12345,2345) || !psu_app_set_limits(7654,1234,PSU_SRC_LCD))exit(2);
                changed=true;
            } else {
                PsuSnapshot s;psu_snapshot(&s);
                if(ui::Theme::dark() || ui::Theme::accentIndex()!=2 || ui::Sound::volume()!=70 ||
                   ui::Sound::enabled() || ui::Sound::tone()!=1 ||
                   psu_preset_get(0)->voltage_mv!=12345 || psu_preset_get(0)->current_ma!=2345 ||
                   s.requested_mv!=7654 || s.requested_ma!=1234 || s.output_requested || s.output_confirmed)exit(3);
                puts("PASS simulator restart: presets, theme, sound, manual limits and output OFF restored");exit(0);
            }
        }
        if(changed && frames>200 && !strcmp(ui::settingsPersistenceStatus(),"Settings saved")) {
            puts("PASS simulator settings committed");exit(0);
        }
        if(frames>600)exit(4);
        return false;
    }
private:
    const char *mode;unsigned frames;bool changed;
};
