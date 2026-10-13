#ifndef BATTERYTRENDWIDGET_HPP
#define BATTERYTRENDWIDGET_HPP
#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/BatteryTrend.hpp>
class BatteryTrendWidget:public touchgfx::Widget {
public:
    unsigned rangeMw()const{unsigned max=1000;const BatteryTrend& h=batteryTrend();for(unsigned i=0;i<h.count;i++){int32_t p=h.at(i);unsigned a=p<0?-(int64_t)p:p;if(a>max)max=a;}return ((max+9999)/10000)*10000;}
    virtual touchgfx::Rect getSolidRect()const{return touchgfx::Rect(0,0,getWidth(),getHeight());}
    virtual void draw(const touchgfx::Rect& area)const{
        auto rect=getAbsoluteRect();auto clip=area;clip.x+=rect.x;clip.y+=rect.y;
        touchgfx::HAL::lcd().fillRect(clip,ui::Theme::color(ui::SURFACE));
        touchgfx::Rect zero(rect.x,rect.y+rect.height/2,rect.width,1);zero&=clip;
        if(!zero.isEmpty())touchgfx::HAL::lcd().fillRect(zero,ui::Theme::color(ui::BORDER));
        const BatteryTrend& h=batteryTrend();const unsigned range=rangeMw();
        for(unsigned i=0;i<h.count;i++){
            int32_t p=h.at(i);int span=(int64_t)(p<0?-p:p)*(rect.height/2-3)/range;
            if(!span)span=1;
            touchgfx::Rect bar(rect.x+(128-h.count+i)*rect.width/128,rect.y+rect.height/2-(p>=0?span:0),3,span);bar&=clip;
            if(!bar.isEmpty())touchgfx::HAL::lcd().fillRect(bar,ui::Theme::color(p>=0?ui::POSITIVE:ui::NEGATIVE));
        }
    }
};
#endif
