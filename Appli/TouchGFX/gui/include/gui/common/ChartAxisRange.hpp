#pragma once
#include <stdint.h>
// Auto range of all visible samples; five-second hysteresis only contracts bounds.
class ChartAxisRange {
public:
 ChartAxisRange(uint32_t span):low(0),high(span),minimumSpan(span),ready(false),armed(false),pendingLow(0),pendingHigh(0),since(0){}
 void reset(){ready=armed=false;}
 static uint32_t nice(uint32_t v){uint32_t decade=1;while(v>10U*decade&&decade<100000000U)decade*=10;return v<=decade?decade:v<=2U*decade?2U*decade:v<=5U*decade?5U*decade:10U*decade;}
 void update(uint32_t minimum,uint32_t maximum,uint32_t now){
  uint32_t span=maximum-minimum;if(span<minimumSpan)span=minimumSpan;
  uint32_t pad=span/10;if(!pad)pad=1;
  uint32_t step=nice((span+2*pad+3)/4);
  uint32_t a=(minimum>pad?minimum-pad:0)/step*step;
  uint32_t b=(maximum+pad+step-1)/step*step;if(b<=a)b=a+step;
  if(!ready){low=a;high=b;ready=true;armed=false;return;}
  if(a<low||b>high){if(a<low)low=a;if(b>high)high=b;armed=false;}
  if(a==low&&b==high){armed=false;return;}
  if(!armed||a!=pendingLow||b!=pendingHigh){pendingLow=a;pendingHigh=b;since=now;armed=true;}
  else if(now-since>=5000U){low=a;high=b;armed=false;}
 }
 uint32_t low,high;
private:
 uint32_t minimumSpan;
 bool ready,armed;
 uint32_t pendingLow,pendingHigh,since;
};
