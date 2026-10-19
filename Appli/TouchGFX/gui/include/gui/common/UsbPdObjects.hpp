#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "g4_pd_caps.h"
namespace ui {
inline bool pdCapabilitiesCurrent(const G4PdCapabilities& c,uint32_t now,bool attached,unsigned role,unsigned generation) {
    return attached&&c.valid&&now-c.ms<=1000u&&(c.flags&1u)&&c.role==role&&
        (uint8_t)c.attach_generation==(uint8_t)generation;
}
struct PdObject {
    uint32_t raw,minMv,maxMv,ma,mw;
    unsigned type,apdo;
    bool valid;
    static PdObject decode(uint32_t raw) {
        PdObject p={raw,0,0,0,0,raw>>30,(raw>>28)&3u,false};
        if(!raw)return p;
        if(p.type==0){p.minMv=p.maxMv=((raw>>10)&1023u)*50u;p.ma=(raw&1023u)*10u;}
        else if(p.type==1||p.type==2){p.minMv=((raw>>10)&1023u)*50u;p.maxMv=((raw>>20)&1023u)*50u;if(p.type==1)p.mw=(raw&1023u)*250u;else p.ma=(raw&1023u)*10u;}
        else if(p.apdo==0){p.minMv=((raw>>8)&255u)*100u;p.maxMv=((raw>>17)&255u)*100u;p.ma=(raw&127u)*50u;}
        else return p;
        p.valid=p.minMv>0&&p.maxMv>=p.minMv&&(p.ma||p.mw);
        return p;
    }
};
inline unsigned selectedPdObject(const G4PdCapabilities& c) {
    const unsigned i=(c.rdo>>28)&15u;
    return (c.flags&4u)&&i>0&&i<=c.count&&i<=7&&c.active_pdo&&c.pdo[i-1]==c.active_pdo?i:0;
}
inline void formatPdRow(char *out,size_t n,uint32_t raw,unsigned index,bool selected) {
    const PdObject p=PdObject::decode(raw);
    const char marker=selected?'>':' ';
    if(p.valid&&p.type==0)snprintf(out,n,"%c %u FIXED  %2lu.%02lu V       %lu.%02lu A",marker,index,(unsigned long)(p.maxMv/1000),(unsigned long)(p.maxMv%1000/10),(unsigned long)(p.ma/1000),(unsigned long)(p.ma%1000/10));
    else if(p.valid)snprintf(out,n,"%c %u %-5s %2lu.%02lu-%2lu.%02lu V %lu.%02lu %s",marker,index,p.type==1?"BATT":p.type==2?"VAR":"PPS",(unsigned long)(p.minMv/1000),(unsigned long)(p.minMv%1000/10),(unsigned long)(p.maxMv/1000),(unsigned long)(p.maxMv%1000/10),(unsigned long)((p.type==1?p.mw:p.ma)/1000),(unsigned long)((p.type==1?p.mw:p.ma)%1000/10),p.type==1?"W":"A");
    else if(p.type==3&&p.apdo==2)snprintf(out,n,"%c %u AVS 9-15V %lu.%02luA /20V %lu.%02luA",marker,index,(unsigned long)(((raw>>10)&1023u)/100),(unsigned long)(((raw>>10)&1023u)%100),(unsigned long)((raw&1023u)/100),(unsigned long)((raw&1023u)%100));
    else snprintf(out,n,"%c %u %s 0x%08lX",marker,index,raw?"UNSUPPORTED":"INVALID",(unsigned long)raw);
}
inline void formatPdRequest(char *out,size_t n,const G4PdCapabilities& c) {
    const unsigned index=(c.rdo>>28)&15u;
    if(!(c.flags&4u)){snprintf(out,n,"No active RDO\nWaiting for a PD contract");return;}
    const uint32_t raw=c.active_pdo;
    const PdObject p=PdObject::decode(raw);
    const char *flags=(c.rdo&(1u<<26))?"CAP MISMATCH":(c.rdo&(1u<<25))?"USB COMM":"ACTIVE";
    if(p.valid&&p.type==3&&p.apdo==0) {
        const uint32_t mv=((c.rdo>>9)&4095u)*20u,ma=(c.rdo&127u)*50u;
        snprintf(out,n,"PDO #%u / PPS %lu.%02lu V\nOperating %lu.%02lu A\n0x%08lX / %s",index,(unsigned long)(mv/1000),(unsigned long)(mv%1000/10),(unsigned long)(ma/1000),(unsigned long)(ma%1000/10),(unsigned long)c.rdo,flags);
    } else if(p.valid) {
        const uint32_t scale=p.type==1?250u:10u,op=((c.rdo>>10)&1023u)*scale,limit=(c.rdo&1023u)*scale;
        snprintf(out,n,"PDO #%u / %s %lu.%02lu %s\n%s %lu.%02lu %s / %s\nRDO 0x%08lX",index,p.type==1?"Operating":"Current",(unsigned long)(op/1000),(unsigned long)(op%1000/10),p.type==1?"W":"A",(c.rdo&(1u<<27))?"Min":"Max",(unsigned long)(limit/1000),(unsigned long)(limit%1000/10),p.type==1?"W":"A",flags,(unsigned long)c.rdo);
    } else snprintf(out,n,"PDO #%u / active request\nRDO 0x%08lX\n%s",index,(unsigned long)c.rdo,flags);
}
}
