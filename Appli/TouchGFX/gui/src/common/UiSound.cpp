#include <gui/common/UiSound.hpp>
extern "C" {
#include "psu_app.h"
}
#ifndef SIMULATOR
#include "board_panel.h"
#endif
namespace { unsigned level=16, pitch=0, responses=3; bool audible=true;
void restoreSound(){PsuAppearance a;psu_appearance_get(&a);level=a.sound_volume;audible=a.sound_enabled!=0;pitch=a.sound_tone;responses=a.sound_feedback;}
void saveSound(){PsuAppearance a;psu_appearance_get(&a);a.sound_volume=(uint8_t)level;a.sound_enabled=audible;a.sound_tone=(uint8_t)pitch;a.sound_feedback=(uint8_t)responses;psu_appearance_set(&a);}
}
namespace ui {
void Sound::sync(){restoreSound();apply();}
unsigned Sound::volume(){restoreSound();return level;}
bool Sound::enabled(){restoreSound();return audible && level;}
unsigned Sound::tone(){restoreSound();return pitch;}
unsigned Sound::feedback(){restoreSound();return responses;}
void Sound::apply(){
    saveSound();
#ifndef SIMULATOR
    BoardBuzzer_Configure(audible?level:0,pitch,responses);
#endif
}
void Sound::setVolume(unsigned value){restoreSound();level=value>100?100:value;audible=level!=0;apply();}
void Sound::toggle(){restoreSound();audible=!audible;if(audible&&!level)level=16;apply();}
void Sound::cycleTone(){restoreSound();pitch=(pitch+1)%3;apply();}
void Sound::cycleFeedback(){restoreSound();responses=responses==3?1:responses==1?2:3;apply();}
void Sound::test(){
#ifndef SIMULATOR
    if(enabled())BoardBuzzer_Request(350);
#endif
}
void Sound::touch(){
#ifndef SIMULATOR
    if(enabled()&&(responses&1))BoardBuzzer_Request(20);
#endif
}
}
