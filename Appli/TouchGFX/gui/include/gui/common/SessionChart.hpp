#pragma once
#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/Color.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/ChartAxisRange.hpp>
extern "C" {
#include "psu_app.h"
}
class SessionChart: public touchgfx::Widget {
public:
 SessionChart():voltageMin(0),voltageMax(1000),currentMin(0),currentMax(100),timeMax(10000),session(0xffffffffU),voltageRange(200),currentRange(100),liveMv(0),liveMa(0),liveValid(false){}
 static touchgfx::colortype voltageColor(){return touchgfx::Color::getColorFromRGB(45,125,235);}
 static touchgfx::colortype currentColor(){return ui::Theme::color(ui::POSITIVE);}
 static uint32_t nice(uint32_t v){uint32_t decade=1;while(v>10U*decade&&decade<100000000U)decade*=10;return v<=decade?decade:v<=2U*decade?2U*decade:v<=5U*decade?5U*decade:10U*decade;}
 // Continuous full-session time range: no 60 -> 120 s jump compressing history.
 static uint32_t timeRange(uint32_t ms){uint32_t range=ms+ms/10;return range<10000?10000:range;}
 void update(const PsuCharger* c){
  if(session!=c->session_start_ms){session=c->session_start_ms;voltageRange.reset();currentRange.reset();}
  uint32_t minV=0xffffffffU,maxV=0,minI=0xffffffffU,maxI=0;
  for(unsigned k=0;k<c->trace_count;k++){
   if(c->trace_mv[k]<minV)minV=c->trace_mv[k];
   if(c->trace_mv[k]>maxV)maxV=c->trace_mv[k];
   if(c->trace_ma[k]<minI)minI=c->trace_ma[k];
   if(c->trace_ma[k]>maxI)maxI=c->trace_ma[k];
  }
  timeMax=timeRange(c->elapsed_ms);
  PsuSnapshot live;psu_snapshot(&live);
  liveValid=c->running&&live.g0_connected&&!live.g0_stale&&live.current_valid;
  liveMv=live.vout_mv;liveMa=live.display_current_ua>0?(uint32_t)(live.display_current_ua/1000):0;
  if(liveValid){if(liveMv<minV)minV=liveMv;if(liveMv>maxV)maxV=liveMv;if(liveMa<minI)minI=liveMa;if(liveMa>maxI)maxI=liveMa;}
  if(minV!=0xffffffffU){voltageRange.update(minV,maxV,psu_app_now());currentRange.update(minI,maxI,psu_app_now());}
  voltageMin=voltageRange.low;voltageMax=voltageRange.high;currentMin=currentRange.low;currentMax=currentRange.high;
  invalidate();
 }
 bool hasLiveTail() const{return liveValid;}
 uint32_t voltageMin,voltageMax,currentMin,currentMax,timeMax;
 virtual touchgfx::Rect getSolidRect() const {return touchgfx::Rect(0,0,getWidth(),getHeight());}
 virtual void draw(const touchgfx::Rect& invalid) const {
  touchgfx::Rect origin=getAbsoluteRect();touchgfx::Rect clip=invalid;clip.x+=origin.x;clip.y+=origin.y;
  touchgfx::HAL::lcd().fillRect(clip,ui::Theme::color(ui::SURFACE));
  for(int i=0;i<=4;i++){
   line(0,i*(getHeight()-1)/4,getWidth()-1,i*(getHeight()-1)/4,ui::Theme::color(ui::BORDER),clip,origin);
   line(i*(getWidth()-1)/4,0,i*(getWidth()-1)/4,getHeight()-1,ui::Theme::color(ui::BORDER),clip,origin);
  }
  const PsuCharger* c=psu_charger();unsigned n=c->trace_count;
  for(unsigned i=1;i<n;i++){
   int x0=(uint64_t)c->trace_time[i-1]*(getWidth()-1)/timeMax,x1=(uint64_t)c->trace_time[i]*(getWidth()-1)/timeMax;
   line(x0,y(c->trace_mv[i-1],voltageMin,voltageMax),x1,y(c->trace_mv[i],voltageMin,voltageMax),voltageColor(),clip,origin);
   line(x0,y(c->trace_ma[i-1],currentMin,currentMax),x1,y(c->trace_ma[i],currentMin,currentMax),currentColor(),clip,origin);
  }
  // The tail follows actual fresh telemetry between the one-second history samples.
  if(liveValid&&n){
   unsigned last=n-1;
   int x0=(uint64_t)c->trace_time[last]*(getWidth()-1)/timeMax,x1=(uint64_t)c->elapsed_ms*(getWidth()-1)/timeMax;
   line(x0,y(c->trace_mv[last],voltageMin,voltageMax),x1,y(liveMv,voltageMin,voltageMax),voltageColor(),clip,origin);
   line(x0,y(c->trace_ma[last],currentMin,currentMax),x1,y(liveMa,currentMin,currentMax),currentColor(),clip,origin);
  }
 }
private:
 uint32_t session;
 ChartAxisRange voltageRange,currentRange;
 uint32_t liveMv,liveMa;
 bool liveValid;
 int y(uint32_t value,uint32_t lo,uint32_t hi) const{return getHeight()-1-(int64_t(value)-lo)*(getHeight()-1)/(hi-lo);}
 static void line(int x0,int y0,int x1,int y1,touchgfx::colortype color,const touchgfx::Rect& clip,const touchgfx::Rect& origin){
  int dx=x1>x0?x1-x0:x0-x1,sx=x0<x1?1:-1,dy=-(y1>y0?y1-y0:y0-y1),sy=y0<y1?1:-1,err=dx+dy;
  for(;;){touchgfx::Rect pixel(origin.x+x0,origin.y+y0,2,2);pixel&=clip;if(!pixel.isEmpty())touchgfx::HAL::lcd().fillRect(pixel,color);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}
 }
};
