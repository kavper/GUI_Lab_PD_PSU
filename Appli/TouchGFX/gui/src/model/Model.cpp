#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>
extern "C" {
#include "psu_app.h"
}

static void publish(ModelListener *listener)
{
    PsuSnapshot snap;
    if (listener == 0)
        return;
    psu_snapshot(&snap);
    listener->ldoTelemetryUpdated(
        snap.vin_mv, snap.vout_mv, snap.signed_current_ua,
        static_cast<int16_t>(snap.mos_centi),
        static_cast<int16_t>(snap.pcb_centi),
        snap.mode_cc ? 2U : 1U, snap.g0_connected != 0,
        snap.output_confirmed != 0, snap.current_valid != 0,
        snap.current_valid != 0);
}

Model::Model() : modelListener(0)
{
}

void Model::tick()
{
    psu_app_ensure();
#ifdef SIMULATOR
    static uint32_t sim_ms;
    sim_ms += 16U;
    psu_sim_tick(sim_ms);
    psu_app_tick(sim_ms);
#endif
    // Keep receiving and processing data, but freeze widget invalidations until
    // the wipe has finished. The next tick publishes the latest snapshot.
    const FrontendApplication* app = static_cast<const FrontendApplication*>(touchgfx::Application::getInstance());
    if (!app || !app->isScreenTransitionActive())
        publish(modelListener);
}
