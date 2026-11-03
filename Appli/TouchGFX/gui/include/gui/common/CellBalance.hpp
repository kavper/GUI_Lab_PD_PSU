#ifndef CELLBALANCE_HPP
#define CELLBALANCE_HPP
#include <stdint.h>
// Cell imbalance bars that glide instead of jumping. refresh() feeds each
// cell's deviation from the pack average; step() runs every frame and eases
// the bars towards it. The full scale follows the largest deviation: it grows
// quickly so bars do not clip, and relaxes slowly so noise does not rescale.
namespace cellbalance {
// Signed permille of the full scale (-1000..1000).
inline int permille(int64_t deviation, int64_t scale){
    if(scale<=0)return 0;
    int64_t p=deviation*1000/scale;
    return (int)(p>1000?1000:p<-1000?-1000:p);
}
template<unsigned N>
class Animator {
public:
    enum { MIN_SCALE_UV = 5000, EASE = 15, RISE = 6, FALL = 180 };
    Animator():scaleUv(MIN_SCALE_UV),goalScaleUv(MIN_SCALE_UV),primed(false){for(unsigned i=0;i<N;i++){shownUv[i]=goalUv[i]=0;valid[i]=false;}}
    // One telemetry sample; deviationMv only matters where usable is true.
    void target(const bool* usable,const int64_t* deviationMv){
        int32_t maxUv=0;
        for(unsigned i=0;i<N;i++){
            valid[i]=usable[i];
            goalUv[i]=usable[i]?(int32_t)(deviationMv[i]*1000):0;
            const int32_t a=goalUv[i]<0?-goalUv[i]:goalUv[i];
            if(a>maxUv)maxUv=a;
        }
        goalScaleUv=maxUv+maxUv/4;
        if(goalScaleUv<MIN_SCALE_UV)goalScaleUv=MIN_SCALE_UV;
        if(!primed){for(unsigned i=0;i<N;i++)shownUv[i]=goalUv[i];scaleUv=goalScaleUv;primed=true;}
    }
    void step(){
        for(unsigned i=0;i<N;i++)shownUv[i]=approach(shownUv[i],goalUv[i],EASE);
        scaleUv=approach(scaleUv,goalScaleUv,goalScaleUv>scaleUv?RISE:FALL);
    }
    bool usable(unsigned i)const{return valid[i];}
    int permille(unsigned i)const{return cellbalance::permille(shownUv[i],scaleUv);}
    int32_t scaleMv()const{return (scaleUv+500)/1000;}
private:
    static int32_t approach(int32_t now,int32_t goal,int32_t div){
        const int32_t d=goal-now;
        if(d==0)return now;
        int32_t s=d/div;
        if(s==0)s=d>0?1:-1;
        return now+s;
    }
    int32_t shownUv[N],goalUv[N],scaleUv,goalScaleUv;
    bool valid[N],primed;
};
}
#endif
