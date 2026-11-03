#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP
#include <gui_generated/common/FrontendApplicationBase.hpp>
#include <gui/common/FingerSwipeState.hpp>
#include <gui/common/Kit.hpp>
class FrontendHeap;
using namespace touchgfx;

// Navigation model of the unified UI:
//  - pages belong to groups (Monitor, Tools, System); a horizontal finger
//    swipe moves to the neighbouring page of the same group,
//  - a downward swipe from the top edge opens the quick menu sheet,
//  - the header back button returns home (or closes the menu).
class FrontendApplication : public FrontendApplicationBase
{
public:
    enum Page {
        OTHER, SPLASH, MAIN, MENU,
        BATTERY, BMS, USB_PD, OUTPUT, SENSE, PROTECTION,
        PRESETS, SEQUENCER, CHARGER,
        DIAGNOSTICS, NETWORK, APPEARANCE, SERVICE,
        PAGE_COUNT
    };
    enum Motion { FORWARD, BACKWARD, DOWN, UP, INSTANT };
    // Legacy names still used by a few views.
    static const Page SETTINGS = MENU;
    typedef Page SwipePage;

    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }
    virtual void handleTickEvent();
    virtual void handlePendingScreenTransition();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleDragEvent(const touchgfx::DragEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);

    void go(Page target, Motion motion = FORWARD);
    void openMenu();
    void closeMenu();
    void navigateBack();
    static kit::PageInfo pageInfo(Page page);
    static Page neighbour(Page page, int direction);

    bool isScreenTransitionActive() const
    {
        return swipe.active() || (currentTransition && !currentTransition->isDone());
    }
    void setSwipePage(Page value) { page = value; }
    Page swipePage() const { return page; }
    FingerSwipeState& fingerSwipe() { return swipe; }

    // Compatibility wrappers for existing call sites.
    void gotoScreen1ScreenWipeTransitionEast() { go(MAIN, BACKWARD); }
    void gotoScreenSettingsScreenWipeTransitionWest() { openMenu(); }
    void gotoScreenSettingsScreenWipeTransitionEast() { openMenu(); }
    void gotoScreenMeasurementsScreenWipeTransitionWest() { go(OUTPUT); }
    void gotoScreenDiagnosticsScreenWipeTransitionWest() { go(DIAGNOSTICS); }
    void gotoScreenPresetsScreenWipeTransitionWest() { go(PRESETS); }
    void gotoScreenSequencerScreenWipeTransitionWest() { go(SEQUENCER); }
    void gotoScreenBmsScreenWipeTransitionWest() { go(BMS); }
    void gotoScreenBatteryScreenWipeTransitionWest() { go(BATTERY); }
    void gotoScreenBatteryScreenWipeTransitionEast() { go(BATTERY, BACKWARD); }
    void gotoScreenPowerPathScreenWipeTransitionWest() { go(SENSE); }
    void gotoScreenUsbPdScreenWipeTransitionWest() { go(USB_PD); }
    void gotoScreenExtChargerScreenWipeTransitionWest() { go(CHARGER); }
    void gotoScreenNetworkScreenWipeTransitionWest() { go(NETWORK); }
    void gotoScreenAppearanceScreenWipeTransitionWest() { go(APPEARANCE); }
    void gotoScreenProtectionScreenWipeTransitionWest() { go(PROTECTION); }
    void gotoScreenServiceScreenWipeTransitionWest() { go(SERVICE); }

private:
    Page page, origin, destination, menuReturn, pendingPage;
    Motion pendingMotion;
    FingerSwipeState swipe;
    int16_t startX, startY;
    bool armed, rollbackQueued, menuGesture;
    int16_t snapX, snapY; // touch redirected to a nearby control
    MainSwipeEditorState editorState;
    char usbNotice[100];
    uint16_t usbNoticeTicks;
    uint8_t appliedBrightness;
    touchgfx::Callback<FrontendApplication> fingerTransitionCallback;
    touchgfx::Callback<FrontendApplication> routeCallback;
    void beginFingerTransition();
    void rollbackFingerTransition();
    void saveOriginState();
    void performRoute();
    void startSheet(bool open, int pixels, bool autoplay = false);
    template <class Transition> void makeScreen(Page target);
    void applyBrightness();
};
#endif
