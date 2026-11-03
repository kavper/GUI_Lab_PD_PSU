#ifndef CELLBALANCE_HPP
#define CELLBALANCE_HPP
#include <stdint.h>
// Dynamic full scale for cell imbalance bars: the smallest step that holds the
// largest deviation from the average, so a few mV still reads as a visible bar.
namespace cellbalance {
inline int32_t scaleMv(int64_t maxDeviationMv){
    static const int32_t steps[]={5,10,20,50,100,200,500,1000};
    if(maxDeviationMv<0)maxDeviationMv=-maxDeviationMv;
    for(unsigned i=0;i<sizeof(steps)/sizeof(steps[0]);i++)if(maxDeviationMv<=steps[i])return steps[i];
    return 1000;
}
// Signed permille of the full scale (-1000..1000).
inline int permille(int64_t deviationMv,int32_t scale){
    int64_t p=deviationMv*1000/scale;
    return (int)(p>1000?1000:p<-1000?-1000:p);
}
}
#endif
