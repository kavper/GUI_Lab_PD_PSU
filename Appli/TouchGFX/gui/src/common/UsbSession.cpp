#include <gui/common/UsbSession.hpp>
#include <gui/common/TelemetryData.hpp>
UsbSession& usbSession(){static UsbSession session;return session;}
UsbPortPower usbPortPower(const TelemetryRecord& usb,int role){
    UsbPortPower p={false,role==2,false,0,0,0};
    if(!usb.is("bq_ok",1))return p;
    int64_t mv=0,ma=0;
    if(role==1){
        if(usb.get("bq_vbus_mv",mv)&&mv>=0&&mv<=100000&&usb.get("bq_iin_ma",ma)&&ma>=0&&ma<=10000){
            p.valid=true;p.mv=mv;p.ma=ma;p.mw=mv*ma/1000;
        }
    }else if(role==2){
        int64_t vbat=0,idchg=0,vbus=0;
        if(usb.get("bq_vbat_mv",vbat)&&vbat>0&&vbat<=30000&&usb.get("bq_idchg_ma",idchg)&&idchg>=0&&idchg<=20000){
            p.valid=true;p.estimated=true;p.mw=vbat*idchg/1000;
            if((usb.get("tps_vbus_mv",vbus)||usb.get("bq_vbus_mv",vbus))&&vbus>=3000&&vbus<=50000){p.mv=vbus;p.ma=p.mw*1000/vbus;}
        }
    }
    return p;
}
void updateUsbSession(){
    TelemetryRecord usb(G4_RECORD_TC);
    int64_t role=0,generation=0;
    usb.get("attach_gen",generation);
    const bool attached=usb.is("plug",1);
    usb.get("pd_role",role);
    const UsbPortPower p=usbPortPower(usb,(int)role);
    const bool valid=attached&&p.valid;
    usbSession().update(usb.fresh,attached,valid,usb.record.ms,valid?(uint32_t)p.mw:0,(uint8_t)generation);
}
