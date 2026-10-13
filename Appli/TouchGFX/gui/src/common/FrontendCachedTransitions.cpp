#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/common/CachedWipeTransition.hpp>
#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/screensettings_screen/ScreenSettingsView.hpp>
#include <gui/screensettings_screen/ScreenSettingsPresenter.hpp>
#include <gui/screendiagnostics_screen/ScreenDiagnosticsView.hpp>
#include <gui/screendiagnostics_screen/ScreenDiagnosticsPresenter.hpp>
#include <gui/screenpresets_screen/ScreenPresetsView.hpp>
#include <gui/screenpresets_screen/ScreenPresetsPresenter.hpp>
#include <gui/screensequencer_screen/ScreenSequencerView.hpp>
#include <gui/screensequencer_screen/ScreenSequencerPresenter.hpp>
#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
#include <gui/screenbattery_screen/ScreenBatteryPresenter.hpp>
#include <gui/screenpowerpath_screen/ScreenPowerPathView.hpp>
#include <gui/screenpowerpath_screen/ScreenPowerPathPresenter.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdPresenter.hpp>
#include <gui/screenextcharger_screen/ScreenExtChargerView.hpp>
#include <gui/screenextcharger_screen/ScreenExtChargerPresenter.hpp>
#include <gui/screennetwork_screen/ScreenNetworkView.hpp>
#include <gui/screennetwork_screen/ScreenNetworkPresenter.hpp>
#include <gui/screenappearance_screen/ScreenAppearanceView.hpp>
#include <gui/screenappearance_screen/ScreenAppearancePresenter.hpp>

#define CACHED_ROUTE(screen, direction, name) \
void FrontendApplication::name() { \
    cachedTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::name##CachedImpl); \
    pendingScreenTransitionCallback = &cachedTransitionCallback; \
} \
void FrontendApplication::name##CachedImpl() { \
    touchgfx::makeTransition<screen##View, screen##Presenter, CachedWipeTransition<touchgfx::direction>, Model>( \
        &currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model); \
}
CACHED_ROUTE(Screen1, EAST, gotoScreen1ScreenWipeTransitionEast)
CACHED_ROUTE(ScreenSettings, WEST, gotoScreenSettingsScreenWipeTransitionWest)
CACHED_ROUTE(ScreenSettings, EAST, gotoScreenSettingsScreenWipeTransitionEast)
CACHED_ROUTE(ScreenDiagnostics, WEST, gotoScreenDiagnosticsScreenWipeTransitionWest)
CACHED_ROUTE(ScreenPresets, WEST, gotoScreenPresetsScreenWipeTransitionWest)
CACHED_ROUTE(ScreenSequencer, WEST, gotoScreenSequencerScreenWipeTransitionWest)
CACHED_ROUTE(ScreenBattery, WEST, gotoScreenBatteryScreenWipeTransitionWest)
CACHED_ROUTE(ScreenPowerPath, WEST, gotoScreenPowerPathScreenWipeTransitionWest)
CACHED_ROUTE(ScreenUsbPd, WEST, gotoScreenUsbPdScreenWipeTransitionWest)
CACHED_ROUTE(ScreenExtCharger, WEST, gotoScreenExtChargerScreenWipeTransitionWest)
CACHED_ROUTE(ScreenNetwork, WEST, gotoScreenNetworkScreenWipeTransitionWest)
CACHED_ROUTE(ScreenAppearance, WEST, gotoScreenAppearanceScreenWipeTransitionWest)
#undef CACHED_ROUTE
