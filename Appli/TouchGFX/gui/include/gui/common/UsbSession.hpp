#ifndef USBSESSION_HPP
#define USBSESSION_HPP
#include <stdint.h>
// Integrate distinct valid input ADC samples only; no energy across stale gaps.
class UsbSession {
public:
    UsbSession():energyMwMs(0),elapsedMs(0),peakMw(0),previousMs(0),previousMw(0),connected(false),primed(false),hasData(false),generation(0){}
    void update(bool fresh,bool attached,bool valid,uint32_t sampleMs,uint32_t powerMw,uint8_t attachGeneration=0){
        if(!fresh){primed=false;return;}
        if(attached&&(!connected||attachGeneration!=generation)){energyMwMs=0;elapsedMs=0;peakMw=0;hasData=false;primed=false;}
        connected=attached;generation=attachGeneration;
        if(!attached||!valid){primed=false;return;}
        if(primed&&sampleMs==previousMs)return;
        if(powerMw>peakMw)peakMw=powerMw;
        if(primed){uint32_t dt=sampleMs-previousMs;if(dt<=1000){energyMwMs+=(uint64_t)(previousMw+powerMw)*dt/2;elapsedMs+=dt;}}
        previousMs=sampleMs;previousMw=powerMw;primed=true;hasData=true;
    }
    uint64_t energyMwMs;
    uint32_t elapsedMs,peakMw;
    bool hasSamples()const{return hasData;}
private:
    uint32_t previousMs,previousMw;
    bool connected,primed,hasData;
    uint8_t generation;
};
UsbSession& usbSession();
void updateUsbSession();
#endif
