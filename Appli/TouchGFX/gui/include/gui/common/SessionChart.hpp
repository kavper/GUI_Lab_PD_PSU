#pragma once
#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/Color.hpp>
extern "C" {
#include "psu_app.h"
}
class SessionChart: public touchgfx::Widget {
public:
 virtual touchgfx::Rect getSolidRect() const {return touchgfx::Rect(0,0,getWidth(),getHeight());}
 virtual void draw(const touchgfx::Rect& invalid) const {
  touchgfx::Rect origin=getAbsoluteRect();touchgfx::Rect clip=invalid;clip.x+=origin.x;clip.y+=origin.y;
  touchgfx::HAL::lcd().fillRect(clip,touchgfx::Color::getColorFromRGB(255,255,255));
  for(int i=0;i<=4;i++)line(0,i*(getHeight()-1)/4,getWidth()-1,i*(getHeight()-1)/4,touchgfx::Color::getColorFromRGB(225,231,239),clip,origin);
  const PsuCharger* c=psu_charger();unsigned n=c->trace_count;if(n<2)return;
  uint32_t duration=c->elapsed_ms?c->elapsed_ms:1;
  uint32_t maxV=c->profile.cells*c->profile.cv_mv_cell*11/10; if(!maxV)maxV=5000;
  uint32_t maxI=c->profile.cc_ma*12/10;if(!maxI)maxI=1000;
  for(unsigned i=0;i<n;i++){if(c->trace_mv[i]>maxV)maxV=c->trace_mv[i];if(c->trace_ma[i]>maxI)maxI=c->trace_ma[i];}
  for(unsigned i=1;i<n;i++){
   int x0=(uint64_t)c->trace_time[i-1]*(getWidth()-1)/duration,x1=(uint64_t)c->trace_time[i]*(getWidth()-1)/duration;
   line(x0,getHeight()-1-(uint64_t)c->trace_mv[i-1]*(getHeight()-1)/maxV,x1,getHeight()-1-(uint64_t)c->trace_mv[i]*(getHeight()-1)/maxV,touchgfx::Color::getColorFromRGB(36,87,230),clip,origin);
   line(x0,getHeight()-1-(uint64_t)c->trace_ma[i-1]*(getHeight()-1)/maxI,x1,getHeight()-1-(uint64_t)c->trace_ma[i]*(getHeight()-1)/maxI,touchgfx::Color::getColorFromRGB(16,139,115),clip,origin);
  }
 }
private:
 static void line(int x0,int y0,int x1,int y1,touchgfx::colortype color,const touchgfx::Rect& clip,const touchgfx::Rect& origin){
  int dx=x1>x0?x1-x0:x0-x1,sx=x0<x1?1:-1,dy=-(y1>y0?y1-y0:y0-y1),sy=y0<y1?1:-1,err=dx+dy;
  for(;;){touchgfx::Rect pixel(origin.x+x0,origin.y+y0,2,2);pixel&=clip;if(!pixel.isEmpty())touchgfx::HAL::lcd().fillRect(pixel,color);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}
 }
};
