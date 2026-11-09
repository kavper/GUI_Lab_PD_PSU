#ifndef USB_POWER_MEASUREMENT_HPP
#define USB_POWER_MEASUREMENT_HPP
#include <stdint.h>
namespace ui {
struct UsbPowerMeasurement { bool valid; uint32_t milliwatts; };
/* RAC/ADCIIN reports the current magnitude in reverse OTG too. Direction
 * comes from the actual TPS role and confirmed BQ operating state. */
inline UsbPowerMeasurement measuredUsbPower(bool fresh, bool attached, int role,
    bool bqReady, bool otg, int64_t mv, int64_t ma) {
    UsbPowerMeasurement out={false,0};
    bool direction=(role==1&&!otg)||(role==2&&otg);
    if (!fresh||!attached||!bqReady||!direction||mv<=0||mv>26000||ma<0||ma>25500)
        return out;
    out.valid=true;out.milliwatts=static_cast<uint32_t>(mv*ma/1000);return out;
}
}
#endif
