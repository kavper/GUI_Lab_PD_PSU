#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP
#include <gui_generated/common/FrontendApplicationBase.hpp>
#include <gui/common/FingerSwipeState.hpp>
class FrontendHeap;
using namespace touchgfx;
class FrontendApplication : public FrontendApplicationBase
{
public:
    enum SwipePage { OTHER, MAIN, SETTINGS, USB_PD };
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }
    virtual void handleTickEvent();
    virtual void handlePendingScreenTransition();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleDragEvent(const touchgfx::DragEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
    bool isScreenTransitionActive() const
    {
        return swipe.active() || (currentTransition && !currentTransition->isDone());
    }
    void setSwipePage(SwipePage value) { page=value; }
    SwipePage swipePage() const { return page; }
    FingerSwipeState& fingerSwipe() { return swipe; }
    // Hide the generated entry points with cached transitions; Designer calls
    // still use the same names and generated sources remain unchanged.
#define CACHED_ROUTE(name) void name(); void name##CachedImpl();
    CACHED_ROUTE(gotoScreen1ScreenWipeTransitionEast)
    CACHED_ROUTE(gotoScreenSettingsScreenWipeTransitionWest)
    CACHED_ROUTE(gotoScreenSettingsScreenWipeTransitionEast)
    CACHED_ROUTE(gotoScreenMeasurementsScreenWipeTransitionWest)
    CACHED_ROUTE(gotoScreenDiagnosticsScreenWipeTransitionWest)
    CACHED_ROUTE(gotoScreenPresetsScreenWipeTransitionWest)
    CACHED_ROUTE(gotoScreenSequencerScreenWipeTransitionWest)
    CACHED_ROUTE(gotoScreenBatteryScreenWipeTransitionWest)
    CACHED_ROUTE(gotoScreenPowerPathScreenWipeTransitionWest)
    CACHED_ROUTE(gotoScreenUsbPdScreenWipeTransitionWest)
    CACHED_ROUTE(gotoScreenExtChargerScreenWipeTransitionWest)
    CACHED_ROUTE(gotoScreenNetworkScreenWipeTransitionWest)
    CACHED_ROUTE(gotoScreenAppearanceScreenWipeTransitionWest)
#undef CACHED_ROUTE
private:
    SwipePage page,origin,destination;
    FingerSwipeState swipe;
    int16_t startX,startY;
    bool armed,rollbackQueued;
    MainSwipeEditorState editorState;
    char usbNotice[100];
    uint16_t usbNoticeTicks;
    touchgfx::Callback<FrontendApplication> fingerTransitionCallback;
    touchgfx::Callback<FrontendApplication> cachedTransitionCallback;
    void beginFingerTransition();
    void rollbackFingerTransition();
    void saveOriginState();
};
#endif
