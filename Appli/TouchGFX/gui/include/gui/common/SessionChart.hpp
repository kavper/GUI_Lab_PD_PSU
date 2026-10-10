#pragma once
#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/Color.hpp>
#include <gui/common/UiTheme.hpp>
extern "C" {
#include "psu_app.h"
}
class SessionChart: public touchgfx::Widget {
public:
 SessionChart():voltageMax(1000),currentMax(100),timeMax(10000),session(0xffffffffU),peakV(0),peakI(0){}
 static touchgfx::colortype voltageColor(){return touchgfx::Color::getColorFromRGB(45,125,235);}
 static touchgfx::colortype currentColor(){return ui::Theme::color(ui::POSITIVE);}
 static uint32_t nice(uint32_t v){uint32_t decade=1;while(v>10U*decade&&decade<100000000U)decade*=10;return v<=decade?decade:v<=2U*decade?2U*decade:v<=5U*decade?5U*decade:10U*decade;}
 static uint32_t niceTime(uint32_t ms){const uint32_t seconds[]={10,30,60,120,300,600,900,1800,3600,7200,14400,28800};for(unsigned k=0;k<sizeof(seconds)/sizeof(seconds[0]);k++)if(ms<=seconds[k]*1000U)return seconds[k]*1000U;return nice(ms);}
 void update(const PsuCharger* c){
  if(session!=c->session_start_ms){session=c->session_start_ms;peakV=peakI=0;}
  uint32_t v=peakV,i=peakI;
  for(unsigned k=0;k<c->trace_count;k++){if(c->trace_mv[k]>v)v=c->trace_mv[k];if(c->trace_ma[k]>i)i=c->trace_ma[k];}
  peakV=v;peakI=i;
  voltageMax=nice(v?v+v/5:1000);currentMax=nice(i?i+i/5:100);
  timeMax=niceTime(c->elapsed_ms);invalidate();
 }
 uint32_t voltageMax,currentMax,timeMax;
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
   line(x0,getHeight()-1-(uint64_t)c->trace_mv[i-1]*(getHeight()-1)/voltageMax,x1,getHeight()-1-(uint64_t)c->trace_mv[i]*(getHeight()-1)/voltageMax,voltageColor(),clip,origin);
   line(x0,getHeight()-1-(uint64_t)c->trace_ma[i-1]*(getHeight()-1)/currentMax,x1,getHeight()-1-(uint64_t)c->trace_ma[i]*(getHeight()-1)/currentMax,currentColor(),clip,origin);
  }
 }
private:
 uint32_t session,peakV,peakI;
 static void line(int x0,int y0,int x1,int y1,touchgfx::colortype color,const touchgfx::Rect& clip,const touchgfx::Rect& origin){
  int dx=x1>x0?x1-x0:x0-x1,sx=x0<x1?1:-1,dy=-(y1>y0?y1-y0:y0-y1),sy=y0<y1?1:-1,err=dx+dy;
  for(;;){touchgfx::Rect pixel(origin.x+x0,origin.y+y0,2,2);pixel&=clip;if(!pixel.isEmpty())touchgfx::HAL::lcd().fillRect(pixel,color);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}
 }
};
