#ifndef BATTERYTREND_HPP
#define BATTERYTREND_HPP
#include <stdint.h>
class BatteryTrend {
public:
    BatteryTrend():head(0),count(0),lastMs(0),hasSample(false){}
    void update(bool valid,uint32_t sampleMs,int32_t mw){
        if(!valid)return;
        if(hasSample && sampleMs-lastMs<500)return;
        power[head]=mw;head=(head+1)%128;if(count<128)++count;lastMs=sampleMs;hasSample=true;
    }
    int32_t at(unsigned i)const{return power[(head+128-count+i)%128];}
    unsigned head,count;
    int32_t power[128];
private:
    uint32_t lastMs;
    bool hasSample;
};
BatteryTrend& batteryTrend();
void updateBatteryTrend();
#endif
