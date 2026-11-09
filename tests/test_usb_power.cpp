#include <gui/common/UsbPowerMeasurement.hpp>
#include <gui/common/UsbSession.hpp>
#include <assert.h>
#include <stdio.h>
int main(){
 auto sink=ui::measuredUsbPower(true,true,1,true,false,19296,4900);
 assert(sink.valid&&sink.milliwatts==94550);
 auto source=ui::measuredUsbPower(true,true,2,true,true,4992,2700);
 assert(source.valid&&source.milliwatts==13478);
 assert(ui::measuredUsbPower(true,true,2,true,true,19968,3600).milliwatts==71884);
 assert(!ui::measuredUsbPower(true,true,2,true,false,5000,1000).valid);
 assert(!ui::measuredUsbPower(true,true,1,true,true,5000,1000).valid);
 assert(!ui::measuredUsbPower(false,true,2,true,true,5000,1000).valid);
 assert(!ui::measuredUsbPower(true,false,2,true,true,5000,1000).valid);
 assert(!ui::measuredUsbPower(true,true,2,false,true,5000,1000).valid);
 assert(!ui::measuredUsbPower(true,true,0,true,true,5000,1000).valid);
 assert(!ui::measuredUsbPower(true,true,2,true,true,0,1000).valid);
 assert(!ui::measuredUsbPower(true,true,2,true,true,5000,-1).valid);
 UsbSession s;s.update(true,true,true,1000,10000,1,1);s.update(true,true,true,1500,10000,1,1);
 assert(s.elapsedMs==500);s.update(true,true,true,2000,20000,1,2);
 assert(s.elapsedMs==0&&s.energyMwMs==0&&s.peakMw==20000);
 s.update(true,true,true,2500,20000,1,2);assert(s.elapsedMs==500&&s.energyMwMs==10000000);
 puts("PASS USB measured source/sink power, state coherence, invalid/stale data, energy reset on role change");
}
