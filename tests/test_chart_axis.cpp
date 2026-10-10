#include <gui/common/ChartAxisRange.hpp>
#include <assert.h>
#include <stdio.h>
int main(){
 ChartAxisRange v(200),i(100);
 v.update(12000,15000,0);i.update(400,1000,0);
 assert(v.low>0&&v.low<12000&&v.high>15000);assert(i.low>0&&i.low<400&&i.high>1000);
 uint32_t oldV=v.high;i.update(1400,3200,10);assert(i.high>3200);assert(v.high==oldV);
 uint32_t peak=i.high;i.update(400,1000,20);i.update(400,1000,5019);assert(i.high==peak);
 i.update(400,1000,5020);assert(i.high<peak&&i.high>1000);
 i.update(0,0,6000);i.update(0,0,11000);assert(i.low==0&&i.high>0);
 v.update(0,27000,12000);assert(v.low==0&&v.high>=27000);
 v.reset();v.update(16400,16400,13000);assert(v.low<16400&&v.high>16400&&v.high-v.low<1000);
 puts("PASS Y autorange: separate V/A min/max, headroom, immediate expansion, five-second contraction, zero and constant signals, session reset");
}
