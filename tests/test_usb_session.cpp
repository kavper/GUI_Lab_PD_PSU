#include <gui/common/UsbSession.hpp>
#include <assert.h>
#include <stdio.h>
int main(){
    UsbSession s;
    s.update(true,true,true,1000,10000);
    s.update(true,true,true,1500,20000);
    assert(s.energyMwMs==7500000 && s.elapsedMs==500 && s.peakMw==20000);
    s.update(true,true,true,1500,20000);assert(s.elapsedMs==500);
    s.update(false,true,false,2500,0);s.update(true,true,true,4000,10000);
    assert(s.elapsedMs==500 && s.energyMwMs==7500000);
    s.update(true,true,false,4500,0);s.update(true,true,true,5000,10000);assert(s.elapsedMs==500);
    s.update(true,false,false,5500,0);s.update(true,true,true,6000,5000);
    assert(s.elapsedMs==0 && s.energyMwMs==0 && s.peakMw==5000);
    s.update(true,true,true,8000,10000);assert(s.elapsedMs==0);
    s.update(true,true,true,8500,10000);assert(s.elapsedMs==500);
    UsbSession wrap;wrap.update(true,true,true,0xfffffff0,1000);wrap.update(true,true,true,0x10,1000);assert(wrap.elapsedMs==32);
    puts("PASS USB session unique samples, trapezoid, stale/invalid gaps, reconnect, wrap");
}
