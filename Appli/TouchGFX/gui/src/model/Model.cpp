#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#ifndef SIMULATOR
extern "C" {
#include "ldo_protocol.h"
}
#endif

Model::Model() : modelListener(0)
{

}

void Model::tick()
{
#ifndef SIMULATOR
    LDO_Telemetry telemetry;
    LDO_GetTelemetry(&telemetry);
    if (modelListener)
        modelListener->ldoTelemetryUpdated(
            telemetry.vin_mv, telemetry.vout_mv, telemetry.iout_ua,
            static_cast<int16_t>(telemetry.temperature_centi_c[0] / 10),
            static_cast<int16_t>(telemetry.temperature_centi_c[3] / 10),
            telemetry.mode, telemetry.connected != 0,
            (telemetry.status_flags & 1U) != 0, telemetry.current_valid != 0,
            telemetry.current_calibrated != 0);
#endif
}
