#ifndef TELEMETRY_DATA_HPP
#define TELEMETRY_DATA_HPP
#include <stdio.h>
#include <string.h>
extern "C" {
#include "psu_app.h"
}

enum TelemetryPage { PAGE_DCDC, PAGE_BATTERY, PAGE_BMS, PAGE_CHARGER, PAGE_USB, PAGE_PATH, PAGE_PROTECTION };
struct TelemetryData {
    char metric[5][32];
    char left[512], right[512], status[100];
    bool fresh, fault;
};

/* Formatting is independent of widgets. Each page reads an atomic record;
   absent/stale keys stay unavailable rather than becoming a plausible zero. */
class TelemetryRecord {
public:
    G4Record record;
    bool fresh;
    explicit TelemetryRecord(unsigned kind) {
        memset(&record, 0, sizeof(record));
        g4_record_snapshot(psu_g4(), static_cast<uint8_t>(kind), &record);
        fresh = record.valid && psu_app_now() - record.ms <= (kind==G4_RECORD_T?50U:1000U);
    }
    bool get(const char* key, int64_t& v) const { return fresh && g4_record_value(&record,key,&v); }
    bool is(const char* key, int64_t expected) const {int64_t v;return get(key,v) && v==expected;}
    void value(char* out, size_t n, const char* key, const char* unit="", unsigned scale=1, unsigned decimals=0) const {
        int64_t v;
        if(!get(key,v)){snprintf(out,n,"--");return;}
        const bool negative=v<0;const uint64_t a=negative?static_cast<uint64_t>(-v):static_cast<uint64_t>(v);
        if(decimals)snprintf(out,n,"%s%llu.%0*llu%s",negative?"-":"",static_cast<unsigned long long>(a/scale),decimals,static_cast<unsigned long long>(a%scale),unit);
        else snprintf(out,n,"%lld%s",static_cast<long long>(v),unit);
    }
    void state(char* out,size_t n,const char* key,const char* on="ON",const char* off="OFF") const {
        int64_t v;snprintf(out,n,"%s",get(key,v)?(v?on:off):"--");
    }
    void row(char* out,size_t n,const char* label,const char* key,const char* unit="",unsigned scale=1,unsigned decimals=0) const {
        char v[48];value(v,sizeof(v),key,unit,scale,decimals);append(out,n,label,v);
    }
    void flag(char* out,size_t n,const char* label,const char* key,const char* on="ON",const char* off="OFF") const {
        char v[48];state(v,sizeof(v),key,on,off);append(out,n,label,v);
    }
    void hex(char* out,size_t n,const char* label,const char* key) const {
        char v[48];int64_t x;if(get(key,x))snprintf(v,sizeof(v),"0x%lX",static_cast<unsigned long>(x));else snprintf(v,sizeof(v),"--");append(out,n,label,v);
    }
    static void append(char* out,size_t n,const char* label,const char* value) {
        size_t used=strlen(out);if(used<n)snprintf(out+used,n-used,"%-19s %s\n",label,value);
    }
};

inline void telemetry_page(TelemetryPage page, TelemetryData& d) {
    memset(&d,0,sizeof(d));
    for(unsigned i=0;i<5;i++)strcpy(d.metric[i],"--");
    const unsigned kind=(page==PAGE_BMS||page==PAGE_BATTERY)?G4_RECORD_TB:(page==PAGE_USB||page==PAGE_CHARGER)?G4_RECORD_TC:G4_RECORD_T;
    TelemetryRecord r(kind);
    PsuSnapshot live;psu_snapshot(&live);
    d.fresh=r.fresh;d.fault=live.fault_latched;
    if(!r.record.valid)snprintf(d.status,sizeof(d.status),"Waiting for %s telemetry",kind==0?"METER":kind==1?"BMS":"PD");
    else snprintf(d.status,sizeof(d.status),"%s / updated %lu ms ago",r.fresh?"LIVE":"STALE - waiting for fresh data",static_cast<unsigned long>(psu_app_now()-r.record.ms));
#define MV(i,k) r.value(d.metric[i],32,k," V",1000,3)
#define MA(i,k) r.value(d.metric[i],32,k," A",1000,3)
#define NUM(i,k,u) r.value(d.metric[i],32,k,u)
#define STATE(i,k,on,off) r.state(d.metric[i],32,k,on,off)
#define LEFT(label,key,unit,scale,dec) r.row(d.left,sizeof(d.left),label,key,unit,scale,dec)
#define RIGHT(label,key,unit,scale,dec) r.row(d.right,sizeof(d.right),label,key,unit,scale,dec)
    switch(page) {
    case PAGE_DCDC:
        MV(0,"vin_mv");MV(1,"vout_mv");MA(2,"i_buck_ma");MA(3,"i_boost_ma");
        LEFT("Buck PWM (TA1)","duty_a_x10"," %",10,1);LEFT("Boost PWM (TC2)","duty_c_x10"," %",10,1);
        r.flag(d.left,sizeof(d.left),"Buck auxiliary","ucc_a");r.flag(d.left,sizeof(d.left),"Boost auxiliary","ucc_c");LEFT("DCDC current","iout_ma"," A",1000,3);
        RIGHT("Rail target","vpre_req_mv"," V",1000,3);RIGHT("Slew command","vpre_cmd_mv"," V",1000,3);
        r.flag(d.right,sizeof(d.right),"Regulation","reg_ok","READY","SETTLING");r.flag(d.right,sizeof(d.right),"Power stage","stage_en");
        r.flag(d.right,sizeof(d.right),"Driver fault","flt","ACTIVE","CLEAR");RIGHT("Start hold","hold_ms"," ms",1,0);
        break;
    case PAGE_BATTERY: {
        TelemetryRecord analog=r;analog.fresh=r.fresh&&r.is("sample",1)&&r.is("bms",1);
        analog.value(d.metric[0],32,"pack_mv"," V",1000,3);analog.value(d.metric[1],32,"i_pack_ma"," A",1000,3);
        analog.value(d.metric[2],32,"dV_mv"," mV");r.value(d.metric[2],32,"soc_permille"," %",10,1);
        if(live.battery_energy_valid)snprintf(d.metric[3],32,"%.3f Wh",(double)live.battery_energy_uwms/3600000000000.0);
        analog.row(d.left,sizeof(d.left),"Lowest cell","min_mv"," V",1000,3);analog.row(d.left,sizeof(d.left),"Highest cell","max_mv"," V",1000,3);
        analog.row(d.left,sizeof(d.left),"Cell sum","sum_mv"," V",1000,3);analog.row(d.left,sizeof(d.left),"Stack voltage","stack_mv"," V",1000,3);
        int64_t current;TelemetryRecord::append(d.left,sizeof(d.left),"Current direction",analog.get("i_pack_ma",current)?(current>0?"CHARGING":current<0?"DISCHARGING":"IDLE"):"--");
        r.flag(d.right,sizeof(d.right),"BMS detected","bms","YES","NO");r.flag(d.right,sizeof(d.right),"Configuration","cfg","READY","NOT READY");
        r.flag(d.right,sizeof(d.right),"Charge FET","chg");r.flag(d.right,sizeof(d.right),"Discharge FET","dsg");r.flag(d.right,sizeof(d.right),"ADC sample","sample","VALID","INVALID");
        r.hex(d.right,sizeof(d.right),"Battery faults","fault");break;
    }
    case PAGE_BMS:
        for(unsigned i=0;i<5;i++){
            char key[12];snprintf(key,sizeof(key),"c%u_mv",i+1);int64_t v;
            if(r.get(key,v)&&v==-1)strcpy(d.metric[i],"UNUSED");
            else if(r.is("sample",1)&&r.is("bms",1))r.value(d.metric[i],32,key," V",1000,3);
        }
        r.hex(d.left,sizeof(d.left),"Safety A","sa");r.hex(d.left,sizeof(d.left),"Safety B","sb");r.hex(d.left,sizeof(d.left),"Safety C","sc");
        r.hex(d.left,sizeof(d.left),"Alarm status","alarm");r.hex(d.left,sizeof(d.left),"Fault flags","fault");r.hex(d.left,sizeof(d.left),"FET status","fet");
        RIGHT("Init step","init_step","",1,0);r.hex(d.right,sizeof(d.right),"VCell mode","vcell_rb");r.hex(d.right,sizeof(d.right),"Battery status","batt");
        RIGHT("Config failures","cfg_fail","",1,0);RIGHT("I2C errors","i2c_err","",1,0);RIGHT("Alert count","alerts","",1,0);
        break;
    case PAGE_CHARGER: {
        TelemetryRecord adc=r;adc.fresh=r.fresh&&r.is("bq_ok",1);
        adc.value(d.metric[0],32,"bq_vbat_mv"," V",1000,3);adc.value(d.metric[1],32,"bq_ibat_ma"," A",1000,3);
        adc.value(d.metric[2],32,"bq_vsys_mv"," V",1000,3);adc.value(d.metric[3],32,"bq_iin_ma"," A",1000,3);
        LEFT("Battery target","bq_vreg_mv"," V",1000,3);LEFT("Charge limit","bq_ichg_set_ma"," A",1000,3);LEFT("Input limit","bq_iin_set_ma"," A",1000,3);
        adc.row(d.left,sizeof(d.left),"USB input","bq_vbus_mv"," V",1000,3);adc.row(d.left,sizeof(d.left),"Charge ADC","bq_ichg_ma"," A",1000,3);adc.row(d.left,sizeof(d.left),"Discharge ADC","bq_idchg_ma"," A",1000,3);
        const char *phase=!r.fresh?"--":!r.is("bq_ok",1)?"OFFLINE":r.is("bq_otg",1)?"OTG SOURCE":r.is("bq_fast",1)?"FAST CHARGE":r.is("bq_pre",1)?"PRECHARGE":r.is("bq_in",1)?"INPUT READY":"NO INPUT";
        TelemetryRecord::append(d.right,sizeof(d.right),"Charger stage",phase);
        r.flag(d.right,sizeof(d.right),"Current limit","bq_iindpm","ACTIVE","CLEAR");r.flag(d.right,sizeof(d.right),"Voltage limit","bq_vindpm","ACTIVE","CLEAR");
        r.hex(d.right,sizeof(d.right),"Charger status","bq_st");r.hex(d.right,sizeof(d.right),"Charger faults","bq_fault");r.flag(d.right,sizeof(d.right),"ADC / device","bq_ok","READY","OFFLINE");
        break;
    }
    case PAGE_USB: {
        MV(0,"pd_mv");MA(1,"pd_ma");int64_t v,a;
        if(r.get("pd_mv",v)&&r.get("pd_ma",a)&&v>=0&&a>=0&&v<=100000&&a<=10000){int64_t mw=v*a/1000;snprintf(d.metric[2],32,"%lld.%02lld W",static_cast<long long>(mw/1000),static_cast<long long>((mw%1000)/10));}
        int64_t role;snprintf(d.metric[3],32,"%s",r.get("pd_role",role)?(role==1?"SINK":role==2?"SOURCE":"NO ROLE"):"--");
        LEFT("Type-C VBUS","tps_vbus_mv"," V",1000,3);r.flag(d.left,sizeof(d.left),"Cable","plug","ATTACHED","DETACHED");
        LEFT("CC1 state","cc1","",1,0);LEFT("CC2 state","cc2","",1,0);LEFT("Connection code","conn","",1,0);r.hex(d.left,sizeof(d.left),"Type-C status","typec");
        r.flag(d.right,sizeof(d.right),"OTG source","bq_otg","ACTIVE","INACTIVE");RIGHT("PD reset count","rst","",1,0);r.flag(d.right,sizeof(d.right),"PD reset","rst_busy","BUSY","IDLE");
        RIGHT("Type-C role code","role","",1,0);RIGHT("USB charger input","bq_vbus_mv"," V",1000,3);RIGHT("Input current","bq_iin_ma"," A",1000,3);
        break;
    }
    case PAGE_PATH: {
        STATE(0,"run","RUNNING","STOPPED");STATE(1,"g0_out","ON","OFF");STATE(2,"permit","ALLOWED","BLOCKED");STATE(3,"rem_sense","REMOTE","LOCAL");
        TelemetryRecord aux(G4_RECORD_AUX);
        aux.row(d.left,sizeof(d.left),"Local output","local_mv"," V",1000,3);
        aux.row(d.left,sizeof(d.left),"Remote plus","remote_p_mv"," V",1000,3);
        aux.row(d.left,sizeof(d.left),"Remote minus","remote_n_mv"," V",1000,3);
        int64_t local,plus,minus,code,flags;char text[48];
        if(aux.get("remote_p_mv",plus)&&aux.get("remote_n_mv",minus))snprintf(text,sizeof(text),"%.3f V",(double)(plus-minus)/1000);else strcpy(text,"--");
        TelemetryRecord::append(d.left,sizeof(d.left),"Load voltage",text);
        if(aux.get("local_mv",local)&&aux.get("remote_p_mv",plus)&&aux.get("remote_n_mv",minus))snprintf(text,sizeof(text),"%ld / %ld mV",(long)(local-plus),(long)minus);else strcpy(text,"--");
        TelemetryRecord::append(d.left,sizeof(d.left),"Drop P / N",text);
        aux.row(d.left,sizeof(d.left),"MOSFET T1","t1"," C",10,1);aux.row(d.left,sizeof(d.left),"Ambient T2","t2"," C",10,1);
        const char *codes[]={"OK","NOT READY","DROP PLUS","DROP SUM","REVERSED","MINUS ON PLUS","SHORT","DROP MINUS","NO SAMPLE","LOCAL HIGH","CV LOAD ERROR"};
        TelemetryRecord::append(d.right,sizeof(d.right),"Sense test",aux.get("sense_code",code)&&code>=0&&code<11?codes[code]:"--");
        if(aux.get("sense_flags",flags)){TelemetryRecord::append(d.right,sizeof(d.right),"Remote request",flags&2?"YES":"NO");TelemetryRecord::append(d.right,sizeof(d.right),"Relay K1",flags&1?"REMOTE":"LOCAL");TelemetryRecord::append(d.right,sizeof(d.right),"Sense fault latch",flags&4?"LATCHED":"CLEAR");}
        aux.row(d.right,sizeof(d.right),"Fan PWM","fan"," %");aux.row(d.right,sizeof(d.right),"Fan speed","fan_rpm"," RPM");
        aux.row(d.right,sizeof(d.right),"Bleeder T3","t3"," C",10,1);aux.row(d.right,sizeof(d.right),"LDO / PCB T4","t4"," C",10,1);
        if(aux.get("sense_code",code))snprintf(d.status,sizeof(d.status),"%s",code==0?"Sense within limits; open wires after K1 activation may be undetected":code==1?"Low output: sense wiring has not been evaluated":"Sense test failed - inspect remote wiring");
        if(live.shutdown_pending)snprintf(d.status,sizeof(d.status),"Stopping PSU - waiting for LDO and DCDC to report off");
        else if(live.shutdown_confirmed)snprintf(d.status,sizeof(d.status),"PSU off confirmed: LDO and DCDC stopped");
        break;
    }
    case PAGE_PROTECTION: {
        STATE(0,"fault","FAULT","CLEAR");STATE(1,"g0_fault","FAULT","CLEAR");STATE(2,"flt","FAULT","CLEAR");STATE(3,"permit","ALLOWED","BLOCKED");
        int64_t f;const char *names[]={"DRIVER","OVERVOLTAGE","OVERCURRENT","LOW INPUT","ADC","BMS"};
        if(r.get("fault",f)){if(f==0)strcpy(d.left,"No G4 fault bits reported.\n");else {for(unsigned i=0;i<6;i++)if(f&(1<<i)){size_t used=strlen(d.left);snprintf(d.left+used,sizeof(d.left)-used,"%s\n",names[i]);}}}
        else strcpy(d.left,"Waiting for fault telemetry.\n");
        r.hex(d.right,sizeof(d.right),"G4 fault mask","fault");r.hex(d.right,sizeof(d.right),"G0 fault mask","g0_fault");RIGHT("Power stage error","ps_err","",1,0);
        RIGHT("G0 UART errors","g0_err","",1,0);r.hex(d.right,sizeof(d.right),"G0 UART flags","g0_uart");
        TelemetryRecord::append(d.right,sizeof(d.right),"HMI fault latch",live.fault_latched?live.fault:"CLEAR");
        break;
    }
    }
#undef MV
#undef MA
#undef NUM
#undef STATE
#undef LEFT
#undef RIGHT
}
#endif
