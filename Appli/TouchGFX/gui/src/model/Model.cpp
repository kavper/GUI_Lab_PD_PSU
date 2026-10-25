#include <gui/model/Model.hpp>
#include <gui/common/UsbSession.hpp>
#include <gui/common/BatteryTrend.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/SettingsPersistence.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/UiSound.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_web_features.h"
}

static void publish(ModelListener *listener)
{
    PsuSnapshot snap;
    if (listener == 0)
        return;
    psu_snapshot(&snap);
    listener->ldoTelemetryUpdated(
        snap.vin_mv, snap.vout_mv, snap.signed_current_ua,
        static_cast<int16_t>(snap.mos_centi / 10),
        static_cast<int16_t>(snap.pcb_centi / 10),
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
    updateUsbSession();
    updateBatteryTrend();
    // Keep receiving and processing data, but freeze widget invalidations until
    // the wipe has finished. The next tick publishes the latest snapshot.
    const FrontendApplication* app = static_cast<const FrontendApplication*>(touchgfx::Application::getInstance());
    if (!app || !app->isScreenTransitionActive()) {
        psu_web_features_poll();
        unsigned remoteEvents=psu_web_features_gui_events();
        if(remoteEvents&1){ui::ThemeScreen::get().apply();ui::Sound::sync();}
        if(remoteEvents&2)ui::Sound::test();
        ui::pollSettingsPersistence();
        publish(modelListener);
    }
}
