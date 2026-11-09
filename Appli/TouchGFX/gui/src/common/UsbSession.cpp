#include <gui/common/UsbSession.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/UsbPowerMeasurement.hpp>
UsbSession& usbSession(){static UsbSession session;return session;}
void updateUsbSession(){
    TelemetryRecord usb(G4_RECORD_TC);
    int64_t role=0,mv=0,ma=0,generation=0;
    usb.get("attach_gen",generation);
    const bool attached=usb.is("plug",1);
    const bool haveAdc=usb.get("bq_vbus_mv",mv)&&usb.get("bq_iin_ma",ma);
    usb.get("pd_role",role);
    const ui::UsbPowerMeasurement measured=ui::measuredUsbPower(
        usb.fresh&&haveAdc,attached,static_cast<int>(role),usb.is("bq_ok",1),
        usb.is("bq_otg",1),mv,ma);
    usbSession().update(usb.fresh,attached,measured.valid,usb.record.ms,
        measured.milliwatts,(uint8_t)generation,(uint8_t)role);
}
