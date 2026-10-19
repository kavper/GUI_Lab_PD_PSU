#include <gui/common/UsbSession.hpp>
#include <gui/common/TelemetryData.hpp>
UsbSession& usbSession(){static UsbSession session;return session;}
void updateUsbSession(){
    TelemetryRecord usb(G4_RECORD_TC);
    int64_t role=0,mv=0,ma=0,generation=0;
    usb.get("attach_gen",generation);
    const bool attached=usb.is("plug",1);
    const bool valid=attached&&usb.get("pd_role",role)&&role==1&&usb.is("bq_ok",1)&&
        usb.get("bq_vbus_mv",mv)&&mv>=0&&mv<=100000&&
        usb.get("bq_iin_ma",ma)&&ma>=0&&ma<=10000;
    usbSession().update(usb.fresh,attached,valid,usb.record.ms,valid?(uint32_t)(mv*ma/1000):0,(uint8_t)generation);
}
