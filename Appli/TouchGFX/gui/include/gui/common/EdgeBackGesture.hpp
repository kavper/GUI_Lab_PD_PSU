#pragma once
#include <stdint.h>
class EdgeBackGesture {
public:
    EdgeBackGesture():active(false),cancelled(false),x0(0),y0(0),dx(0),dy(0){}
    bool active,cancelled;
    int16_t x0,y0,dx,dy;
    void begin(int x,int y,bool enabled){active=enabled&&x>=0&&x<=28&&y>=64;cancelled=false;x0=x;y0=y;dx=dy=0;}
    bool move(int x,int y){
        if(!active)return false;
        dx=x-x0;dy=y-y0;
        if(!cancelled && (dy>20||dy< -20) && (dy>dx||-dy>dx)){active=false;return false;}
        if(dx>12 && dx>2*(dy<0?-dy:dy))cancelled=true;
        return cancelled;
    }
    bool finish(){bool go=active&&cancelled&&dx>=80&&dx>2*(dy<0?-dy:dy);active=false;return go;}
};
