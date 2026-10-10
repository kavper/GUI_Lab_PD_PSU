#include <gui/common/PowerGaugeScale.hpp>
#include <assert.h>
#include <stdio.h>

int main()
{
    PowerGaugeScale s;
    assert(s.range()==100000 && s.position(0)==0);
    s.update(-150000,0,true);
    assert(s.range()==200000 && s.position(-150000)==-258);
    s.update(-250000,20,true);
    assert(s.range()==300000 && s.position(-250000)==-286);
    s.update(-260000,40,true); // Combined loads plus losses fit without clipping.
    assert(s.range()==300000);
    assert(s.position(250000)==286); // Same scale for charging/discharging.
    s.update(-150000,60,true);
    s.update(-150000,5059,true);assert(s.range()==300000);
    s.update(-150000,5060,true);assert(s.range()==200000);
    // Oscillation near a boundary must not switch range every frame.
    for(uint32_t t=5080;t<8000;t+=20)s.update(t%40?175000:173000,t,true);
    assert(s.range()==300000);
    s.update(1000,8000,true);
    s.update(0,12000,false); // Missing telemetry cancels shrink timer.
    s.update(1000,13000,true);assert(s.range()==300000);
    s.update(1000,17999,true);assert(s.range()==300000);
    s.update(1000,18000,true);assert(s.range()==100000);
    s.update(350000,18001,true);assert(s.range()==500000);
    s.update(750000,18002,true);assert(s.range()==1000000);
    s.update(10000000,18003,true);assert(s.range()==20000000);
    assert(s.position(INT64_MAX)==344 && s.position(INT64_MIN)==-344);
    s.update(INT64_MIN,18004,true);assert(s.range()==20000000);
    PowerGaugeScale wrap;
    wrap.update(250000,UINT32_MAX-100,true);
    wrap.update(1000,UINT32_MAX-10,true);
    wrap.update(1000,4989,true);assert(wrap.range()==100000);
    puts("Power gauge range, headroom, hysteresis and sign tests passed");
}
