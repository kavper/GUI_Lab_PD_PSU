#include <gui/common/BatteryTrend.hpp>
#include <assert.h>
#include <stdio.h>
int main(){BatteryTrend t;t.update(false,0,100);assert(t.count==0);t.update(true,100,1000);t.update(true,100,9999);t.update(true,599,9999);assert(t.count==1&&t.at(0)==1000);t.update(true,600,-2000);assert(t.count==2&&t.at(1)==-2000);for(unsigned i=0;i<150;i++)t.update(true,1100+i*500,i);assert(t.count==128&&t.at(127)==149&&t.at(0)==22);BatteryTrend wrap;wrap.update(true,0xffffff00,1);wrap.update(true,0x100,2);assert(wrap.count==2);puts("PASS battery trend valid samples, cadence, duplicate rejection, ring and time wrap");}
