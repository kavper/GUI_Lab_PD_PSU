#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/common/CachedWipeTransition.hpp>
#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/screensettings_screen/ScreenSettingsView.hpp>
#include <gui/screensettings_screen/ScreenSettingsPresenter.hpp>
#include <gui/screendiagnostics_screen/ScreenDiagnosticsView.hpp>
#include <gui/screenmeasurements_screen/ScreenMeasurementsView.hpp>
#include <gui/screenmeasurements_screen/ScreenMeasurementsPresenter.hpp>
#include <gui/screendiagnostics_screen/ScreenDiagnosticsPresenter.hpp>
#include <gui/screenpresets_screen/ScreenPresetsView.hpp>
#include <gui/screenpresets_screen/ScreenPresetsPresenter.hpp>
#include <gui/screensequencer_screen/ScreenSequencerView.hpp>
#include <gui/screensequencer_screen/ScreenSequencerPresenter.hpp>
#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
#include <gui/screenbms_screen/ScreenBmsView.hpp>
#include <gui/screenbms_screen/ScreenBmsPresenter.hpp>
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

#include <gui/screenprotection_screen/ScreenProtectionView.hpp>
#include <gui/screenprotection_screen/ScreenProtectionPresenter.hpp>
#include <gui/screenservice_screen/ScreenServiceView.hpp>
#include <gui/screenservice_screen/ScreenServicePresenter.hpp>

#define CACHED_ROUTE(screen, direction, name, pageType) \
void FrontendApplication::name() { \
    cachedTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::name##CachedImpl); \
    pendingScreenTransitionCallback = &cachedTransitionCallback; \
} \
void FrontendApplication::name##CachedImpl() { \
    backPage = pageType==USB_PD ? (page==BATTERY?BATTERY:page==MAIN?MAIN:SETTINGS) : pageType==BMS && page==BATTERY ? BATTERY : SETTINGS; \
    touchgfx::makeTransition<screen##View, screen##Presenter, CachedWipeTransition<touchgfx::direction>, Model>( \
        &currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model); \
    page=pageType; \
}
CACHED_ROUTE(Screen1, EAST, gotoScreen1ScreenWipeTransitionEast, MAIN)
CACHED_ROUTE(ScreenSettings, WEST, gotoScreenSettingsScreenWipeTransitionWest, SETTINGS)
CACHED_ROUTE(ScreenSettings, EAST, gotoScreenSettingsScreenWipeTransitionEast, SETTINGS)
CACHED_ROUTE(ScreenMeasurements, WEST, gotoScreenMeasurementsScreenWipeTransitionWest, DETAIL)
CACHED_ROUTE(ScreenDiagnostics, WEST, gotoScreenDiagnosticsScreenWipeTransitionWest, DETAIL)
CACHED_ROUTE(ScreenPresets, WEST, gotoScreenPresetsScreenWipeTransitionWest, DETAIL)
CACHED_ROUTE(ScreenSequencer, WEST, gotoScreenSequencerScreenWipeTransitionWest, DETAIL)
CACHED_ROUTE(ScreenBms, WEST, gotoScreenBmsScreenWipeTransitionWest, BMS)
CACHED_ROUTE(ScreenBattery, WEST, gotoScreenBatteryScreenWipeTransitionWest, BATTERY)
CACHED_ROUTE(ScreenPowerPath, WEST, gotoScreenPowerPathScreenWipeTransitionWest, DETAIL)
CACHED_ROUTE(ScreenUsbPd, WEST, gotoScreenUsbPdScreenWipeTransitionWest, USB_PD)
CACHED_ROUTE(ScreenExtCharger, WEST, gotoScreenExtChargerScreenWipeTransitionWest, DETAIL)
CACHED_ROUTE(ScreenNetwork, WEST, gotoScreenNetworkScreenWipeTransitionWest, DETAIL)
CACHED_ROUTE(ScreenAppearance, WEST, gotoScreenAppearanceScreenWipeTransitionWest, DETAIL)
CACHED_ROUTE(ScreenBattery, EAST, gotoScreenBatteryScreenWipeTransitionEast, BATTERY)
CACHED_ROUTE(ScreenProtection, WEST, gotoScreenProtectionScreenWipeTransitionWest, DETAIL)
CACHED_ROUTE(ScreenService, WEST, gotoScreenServiceScreenWipeTransitionWest, DETAIL)
#undef CACHED_ROUTE
