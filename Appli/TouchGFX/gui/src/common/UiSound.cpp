#include <gui/common/UiSound.hpp>
#ifndef SIMULATOR
#include "board_panel.h"
#endif
namespace { unsigned level=16, pitch=0, responses=3; bool audible=true; }
namespace ui {
unsigned Sound::volume(){return level;}
bool Sound::enabled(){return audible && level;}
unsigned Sound::tone(){return pitch;}
unsigned Sound::feedback(){return responses;}
void Sound::apply(){
#ifndef SIMULATOR
    BoardBuzzer_Configure(audible?level:0,pitch,responses);
#endif
}
void Sound::setVolume(unsigned value){level=value>100?100:value;audible=level!=0;apply();}
void Sound::toggle(){audible=!audible;if(audible&&!level)level=16;apply();}
void Sound::cycleTone(){pitch=(pitch+1)%3;apply();}
void Sound::cycleFeedback(){responses=responses==3?1:responses==1?2:3;apply();}
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
