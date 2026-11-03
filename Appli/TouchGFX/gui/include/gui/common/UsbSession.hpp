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
class TelemetryRecord;
// Port power in either direction.
//  SINK:   measured at the charger input (VBUS x IIN).
//  SOURCE: the charger input ADC does not measure the outgoing direction, so
//          the power is the battery discharge (VBAT x IDCHG) feeding the OTG
//          converter; the port current is that power / VBUS (an upper bound,
//          converter losses included) and is flagged as estimated.
struct UsbPortPower { bool valid, source, estimated; int64_t mw, ma, mv; };
UsbPortPower usbPortPower(const TelemetryRecord& usb, int role);
UsbSession& usbSession();
void updateUsbSession();
#endif
