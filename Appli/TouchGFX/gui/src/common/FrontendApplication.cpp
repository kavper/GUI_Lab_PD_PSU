#include <gui/common/UiTheme.hpp>
#include <gui/common/UiSound.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/common/FingerSlideTransition.hpp>
#include <gui/common/CachedWipeTransition.hpp>
#include <gui/common/KitLayout.hpp>
#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/screensettings_screen/ScreenSettingsView.hpp>
#include <gui/screensettings_screen/ScreenSettingsPresenter.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdPresenter.hpp>
#include <gui/screendiagnostics_screen/ScreenDiagnosticsView.hpp>
#include <gui/screendiagnostics_screen/ScreenDiagnosticsPresenter.hpp>
#include <gui/screenmeasurements_screen/ScreenMeasurementsView.hpp>
#include <gui/screenmeasurements_screen/ScreenMeasurementsPresenter.hpp>
#include <gui/screenpresets_screen/ScreenPresetsView.hpp>
#include <gui/screenpresets_screen/ScreenPresetsPresenter.hpp>
#include <gui/screensequencer_screen/ScreenSequencerView.hpp>
#include <gui/screensequencer_screen/ScreenSequencerPresenter.hpp>
#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
#include <gui/screenbattery_screen/ScreenBatteryPresenter.hpp>
#include <gui/screenbms_screen/ScreenBmsView.hpp>
#include <gui/screenbms_screen/ScreenBmsPresenter.hpp>
#include <gui/screenpowerpath_screen/ScreenPowerPathView.hpp>
#include <gui/screenpowerpath_screen/ScreenPowerPathPresenter.hpp>
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
#include <touchgfx/transitions/NoTransition.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <stdlib.h>
extern "C" {
#include "psu_app.h"
}
#ifndef SIMULATOR
#include "board_panel.h"
#include "board_backlight.h"
#include <gui/common/DisplayComposition.hpp>
extern "C" volatile uint32_t BoardGraphicsBenchmark;
extern "C" volatile uint32_t BoardGraphicsSwipeBenchmark;
extern "C" volatile uint32_t BoardGraphicsRouteBenchmark;
extern "C" volatile uint32_t BoardGraphicsMotion;
extern "C" I2C_HandleTypeDef hi2c1;
#endif

namespace {
struct PageMeta { FrontendApplication::Page page; const char* title; int group; int index; };
// Group 0 = Monitor, 1 = Tools, 2 = System.
const PageMeta pages[] = {
    { FrontendApplication::MAIN, "Lab PD PSU", 0, 0 },
    { FrontendApplication::BATTERY, "Battery", 0, 1 },
    { FrontendApplication::BMS, "Cells & BMS", 0, 2 },
    { FrontendApplication::USB_PD, "USB-C · Power Delivery", 0, 3 },
    { FrontendApplication::OUTPUT, "Output path", 0, 4 },
    { FrontendApplication::SENSE, "Sense & temperatures", 0, 5 },
    { FrontendApplication::PROTECTION, "Protection", 0, 6 },
    { FrontendApplication::PRESETS, "Presets", 1, 0 },
    { FrontendApplication::SEQUENCER, "Sequencer", 1, 1 },
    { FrontendApplication::CHARGER, "Cell charger", 1, 2 },
    { FrontendApplication::DIAGNOSTICS, "Diagnostics", 2, 0 },
    { FrontendApplication::NETWORK, "Network", 2, 1 },
    { FrontendApplication::APPEARANCE, "Appearance", 2, 2 },
    { FrontendApplication::SERVICE, "Service", 2, 3 },
};
const char* groupNames[] = { "Monitor", "Tools", "System" };
const int groupSizes[] = { 7, 3, 4 };
const PageMeta* pageMeta(FrontendApplication::Page p)
{
    for (unsigned i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i) if (pages[i].page == p) return &pages[i];
    return 0;
}
bool swipeable(FrontendApplication::Page p) { return pageMeta(p) != 0; }

// Touch targets reach beyond the drawn control: a press that lands next to a
// control (within TOUCH_REACH px) goes to the nearest one. Direct hits win, and
// between two neighbours the closer one wins, so gaps are shared fairly.
const int TOUCH_REACH = 22;
struct NearestTarget { int dist, x, y; bool direct; };
void findTarget(touchgfx::Drawable* parent, int px, int py, NearestTarget& best)
{
    for (touchgfx::Drawable* d = parent->getFirstChild(); d && !best.direct; d = d->getNextSibling()) {
        if (!d->isVisible()) continue;
        if (d->getFirstChild()) findTarget(d, px, py, best);
        if (!d->isTouchable() || best.direct) continue;
        const touchgfx::Rect r = d->getAbsoluteRect();
        if (r.width <= 0 || r.height <= 0) continue;
        if (r.intersect(px, py)) { best.direct = true; return; }
        const int cx = px < r.x ? r.x : px >= r.right() ? r.right() - 1 : px;
        const int cy = py < r.y ? r.y : py >= r.bottom() ? r.bottom() - 1 : py;
        const int dist = (cx - px) * (cx - px) + (cy - py) * (cy - py);
        if (dist <= TOUCH_REACH * TOUCH_REACH && dist < best.dist) { best.dist = dist; best.x = cx; best.y = cy; }
    }
}
}

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap), page(OTHER), origin(OTHER), destination(OTHER), menuReturn(MAIN),
      pendingPage(OTHER), pendingMotion(FORWARD), startX(0), startY(0), armed(false), rollbackQueued(false),
      menuGesture(false), snapX(0), snapY(0), usbNoticeTicks(0), appliedBrightness(0)
{
#ifdef SIMULATOR
    static uint32_t bitmapCache[2 * 1024 * 1024 / 4];
    touchgfx::Bitmap::setCache(reinterpret_cast<uint16_t*>(bitmapCache), sizeof(bitmapCache), 580);
#endif
    usbNotice[0] = 0;
    touchgfx::HAL::getInstance()->setFrameRateCompensation(true);
}

kit::PageInfo FrontendApplication::pageInfo(Page p)
{
    kit::PageInfo info = { "", "", 0, 0, true };
    if (p == MENU) { info.title = "Menu"; info.group = "All pages"; info.back = true; return info; }
    const PageMeta* m = pageMeta(p);
    if (!m) return info;
    info.title = m->title;
    info.group = groupNames[m->group];
    info.index = m->index;
    info.count = groupSizes[m->group];
    info.back = p != MAIN;
    return info;
}

FrontendApplication::Page FrontendApplication::neighbour(Page p, int direction)
{
    const PageMeta* m = pageMeta(p);
    if (!m) return OTHER;
    const int target = m->index + direction;
    for (unsigned i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i)
        if (pages[i].group == m->group && pages[i].index == target) return pages[i].page;
    return OTHER;
}

void FrontendApplication::applyBrightness()
{
    PsuAppearance a;
    psu_appearance_get(&a);
    if (a.brightness == appliedBrightness) return;
#ifndef SIMULATOR
    // GUI task owns I2C1 (shared with GT911 and the settings EEPROM).
    if (!BoardBacklight_SetPercent(&hi2c1, a.brightness)) return;
#endif
    appliedBrightness = a.brightness;
}

void FrontendApplication::handleTickEvent()
{
    model.tick();
    // Hold the settle animation until the transition has its first frame;
    // otherwise the first steps are lost while the destination is rendered.
    bool holdSwipe = pendingScreenTransitionCallback == &fingerTransitionCallback && !rollbackQueued;
#ifndef SIMULATOR
    holdSwipe = holdSwipe || boardDisplayComposition.preparing;
#endif
    if (!holdSwipe) swipe.tick();
    FrontendApplicationBase::handleTickEvent();
    if (!isScreenTransitionActive()) kit::Header::tickActive();
    applyBrightness();
#ifndef SIMULATOR
    BoardGraphicsMotion = isScreenTransitionActive() ? 1 : 0;
    // Opt-in read-only route soak; navigation never changes power settings.
    if(BoardGraphicsRouteBenchmark && !BoardGraphicsSwipeBenchmark && !boardDisplayComposition.resumeFrames && !isScreenTransitionActive() && !pendingScreenTransitionCallback){
        static unsigned route=0;
        typedef void(FrontendApplication::*Route)();
        static const Route routes[]={
            &FrontendApplication::gotoScreenSettingsScreenWipeTransitionWest,
            &FrontendApplication::gotoScreenBatteryScreenWipeTransitionWest,
            &FrontendApplication::gotoScreenSettingsScreenWipeTransitionEast,
            &FrontendApplication::gotoScreenUsbPdScreenWipeTransitionWest,
            &FrontendApplication::gotoScreenSettingsScreenWipeTransitionEast,
            &FrontendApplication::gotoScreenMeasurementsScreenWipeTransitionWest,
            &FrontendApplication::gotoScreenSettingsScreenWipeTransitionEast,
            &FrontendApplication::gotoScreenExtChargerScreenWipeTransitionWest,
            &FrontendApplication::gotoScreenSettingsScreenWipeTransitionEast,
            &FrontendApplication::gotoScreenAppearanceScreenWipeTransitionWest,
            &FrontendApplication::gotoScreenSettingsScreenWipeTransitionEast,
            &FrontendApplication::gotoScreenNetworkScreenWipeTransitionWest,
            &FrontendApplication::gotoScreenSettingsScreenWipeTransitionEast,
            &FrontendApplication::gotoScreenDiagnosticsScreenWipeTransitionWest,
            &FrontendApplication::gotoScreenSettingsScreenWipeTransitionEast,
            &FrontendApplication::gotoScreen1ScreenWipeTransitionEast};
        --BoardGraphicsRouteBenchmark;
        (this->*routes[route++%(sizeof(routes)/sizeof(routes[0]))])();
    }
    // Opt-in SWD benchmark; default zero, never changes output/setpoints.
    if (BoardGraphicsSwipeBenchmark && !boardDisplayComposition.resumeFrames && !isScreenTransitionActive() && !pendingScreenTransitionCallback &&
        (page == MAIN || page == BATTERY)) {
        --BoardGraphicsSwipeBenchmark;
        origin = page; destination = page == MAIN ? BATTERY : MAIN;
        saveOriginState(); rollbackQueued = false;
        const int dir = page == MAIN ? -1 : 1;
        swipe.begin(dir, dir * touchgfx::HAL::DISPLAY_WIDTH * 2 / 5, touchgfx::HAL::DISPLAY_WIDTH);
        swipe.release();
        fingerTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::beginFingerTransition);
        pendingScreenTransitionCallback = &fingerTransitionCallback;
    }
    if (BoardGraphicsBenchmark || boardDisplayComposition.resumeFrames) invalidateArea(touchgfx::Rect(0, 0, touchgfx::HAL::DISPLAY_WIDTH, touchgfx::HAL::DISPLAY_HEIGHT));
    BoardPanelEvent panel;
    while (BoardPanel_Take(&panel)) {
        if (page != MAIN || isScreenTransitionActive() || pendingScreenTransitionCallback) continue;
        Screen1View *view = static_cast<Screen1View*>(currentScreen);
        if (panel.kind == BOARD_VOLTAGE) {
            for (int n = 0; n < abs(panel.delta); ++n) {
                if (panel.delta > 0) view->nudgeVoltUp(); else view->nudgeVoltDown();
            }
        } else if (panel.kind == BOARD_CURRENT) {
            for (int n = 0; n < abs(panel.delta); ++n) {
                if (panel.delta > 0) view->nudgeAmpUp(); else view->nudgeAmpDown();
            }
        } else if (panel.kind == BOARD_VOLTAGE_STEP) { view->selectVoltage(); view->stepVolt(); }
        else if (panel.kind == BOARD_CURRENT_STEP) { view->selectCurrent(); view->stepAmp(); }
    }
#endif
    ui::ThemeScreen::get().sync();
    if (swipe.phase == FingerSwipeState::FINISHED && !rollbackQueued) {
        if (swipe.committed()) {
            if (!currentTransition || currentTransition->isDone()) { page = destination; swipe.reset(); armed = false; }
        }
        else {
            rollbackQueued = true;
            fingerTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::rollbackFingerTransition);
            pendingScreenTransitionCallback = &fingerTransitionCallback;
        }
    }
}

void FrontendApplication::handleClickEvent(const touchgfx::ClickEvent& event)
{
    if (swipe.active()) {
        if (event.getType() == touchgfx::ClickEvent::RELEASED) { swipe.move(swipe.vertical() ? event.getY() - startY : event.getX() - startX); swipe.release(); armed = false; }
        else if (event.getType() == touchgfx::ClickEvent::CANCEL) { swipe.release(true); armed = false; }
        return;
    }
    if (event.getType() == touchgfx::ClickEvent::PRESSED) {
        const bool idle = !isScreenTransitionActive() && !pendingScreenTransitionCallback;
        if (idle) ui::Sound::touch();
        armed = page != OTHER && page != SPLASH && idle;
        startX = event.getX(); startY = event.getY();
        if (armed && page == MAIN) static_cast<Screen1View*>(currentScreen)->saveSwipeEditor(editorState);
        snapX = snapY = 0;
        if (currentScreen) {
            NearestTarget best = { TOUCH_REACH * TOUCH_REACH + 1, 0, 0, false };
            findTarget(&currentScreen->getRootContainer(), event.getX(), event.getY(), best);
            if (!best.direct && best.dist <= TOUCH_REACH * TOUCH_REACH) { snapX = best.x - event.getX(); snapY = best.y - event.getY(); }
        }
    }
    FrontendApplicationBase::handleClickEvent(touchgfx::ClickEvent(event.getType(), event.getX() + snapX, event.getY() + snapY, event.getForce()));
    if (event.getType() != touchgfx::ClickEvent::PRESSED) { armed = false; snapX = snapY = 0; }
}

void FrontendApplication::handleDragEvent(const touchgfx::DragEvent& event)
{
    if (swipe.active()) { swipe.move(swipe.vertical() ? event.getNewY() - startY : event.getNewX() - startX); return; }
    const int dx = event.getNewX() - startX, dy = event.getNewY() - startY;
    // Pull-down menu: starts on the header strip and follows the finger; on
    // the menu itself an upward drag anywhere pushes the sheet back up.
    const bool fromTop = startY < 72 && page != MENU && page != OTHER && page != SPLASH;
    const bool fromSheet = page == MENU;
    if (armed && (fromTop ? dy >= 12 : fromSheet && dy <= -12) && abs(dy) > abs(dx) + abs(dx) / 2 &&
        touchgfx::HAL::USE_ANIMATION_STORAGE) {
        FrontendApplicationBase::handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::CANCEL, startX + snapX, startY + snapY));
        armed = false; snapX = snapY = 0;
        if (fromTop) { menuReturn = swipeable(page) ? page : MAIN; startSheet(true, dy); }
        else startSheet(false, dy);
        return;
    }
    const int direction = dx > 0 ? 1 : -1;
    // Swiping left reveals the next page, right the previous one.
    const Page target = swipeable(page) ? neighbour(page, -direction) : OTHER;
    if (armed && target != OTHER && !kit::swipeBlocked(startX, startY) && abs(dx) >= 14 && abs(dx) > 2 * abs(dy) &&
        touchgfx::HAL::USE_ANIMATION_STORAGE) {
        // Cancel the pressed control before the view is destroyed.
        FrontendApplicationBase::handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::CANCEL, startX + snapX, startY + snapY));
        snapX = snapY = 0;
        origin = page;
        destination = target;
        if (origin != MAIN) saveOriginState();
        rollbackQueued = false;
        swipe.begin(direction, dx, touchgfx::HAL::DISPLAY_WIDTH);
        fingerTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::beginFingerTransition);
        pendingScreenTransitionCallback = &fingerTransitionCallback;
        return;
    }
    FrontendApplicationBase::handleDragEvent(touchgfx::DragEvent(event.getType(), event.getOldX() + snapX, event.getOldY() + snapY,
                                                                 event.getNewX() + snapX, event.getNewY() + snapY));
}

void FrontendApplication::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    if (swipe.active()) {
        if ((event.getType() == touchgfx::GestureEvent::SWIPE_VERTICAL) == swipe.vertical()) swipe.flick(event.getVelocity());
        return;
    }
    FrontendApplicationBase::handleGestureEvent(event);
}

void FrontendApplication::saveOriginState()
{
    if (origin == MAIN) static_cast<Screen1View*>(currentScreen)->saveSwipeEditor(editorState);
    else if (origin == USB_PD) static_cast<ScreenUsbPdView*>(currentScreen)->saveSwipeNotice(usbNotice, usbNoticeTicks);
}

template <class T>
void FrontendApplication::makeScreen(Page target)
{
#define ROUTE(P, S) case P: touchgfx::makeTransition<S##View, S##Presenter, T, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model); break;
    switch (target) {
    ROUTE(MAIN, Screen1)
    ROUTE(MENU, ScreenSettings)
    ROUTE(BATTERY, ScreenBattery)
    ROUTE(BMS, ScreenBms)
    ROUTE(USB_PD, ScreenUsbPd)
    ROUTE(OUTPUT, ScreenMeasurements)
    ROUTE(SENSE, ScreenPowerPath)
    ROUTE(PROTECTION, ScreenProtection)
    ROUTE(PRESETS, ScreenPresets)
    ROUTE(SEQUENCER, ScreenSequencer)
    ROUTE(CHARGER, ScreenExtCharger)
    ROUTE(DIAGNOSTICS, ScreenDiagnostics)
    ROUTE(NETWORK, ScreenNetwork)
    ROUTE(APPEARANCE, ScreenAppearance)
    ROUTE(SERVICE, ScreenService)
    default:
        touchgfx::makeTransition<Screen1View, Screen1Presenter, T, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
        target = MAIN;
        break;
    }
#undef ROUTE
    page = target;
    // Designer edits of the page layout (transitions apply them before capture).
    kit::applyDesignerLayout(target, currentScreen->getRootContainer());
}

void FrontendApplication::beginFingerTransition()
{
    makeScreen<FingerSlideTransition>(destination);
    // The page becomes current only once the swipe commits (see tick).
    page = origin == destination ? origin : destination;
}

void FrontendApplication::rollbackFingerTransition()
{
    makeScreen<touchgfx::NoTransition>(origin);
    if (origin == MAIN) static_cast<Screen1View*>(currentScreen)->restoreSwipeEditor(editorState);
    else if (origin == USB_PD) static_cast<ScreenUsbPdView*>(currentScreen)->restoreSwipeNotice(usbNotice, usbNoticeTicks);
    swipe.reset(); armed = false; rollbackQueued = false;
}

void FrontendApplication::go(Page target, Motion motion)
{
    if (target == OTHER || target == SPLASH) return;
    pendingPage = target;
    pendingMotion = motion;
    routeCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::performRoute);
    pendingScreenTransitionCallback = &routeCallback;
}

void FrontendApplication::performRoute()
{
    switch (pendingMotion) {
    case BACKWARD: makeScreen<CachedWipeTransition<touchgfx::EAST> >(pendingPage); break;
    case DOWN: makeScreen<CachedWipeTransition<touchgfx::NORTH> >(pendingPage); break;
    case UP: makeScreen<CachedWipeTransition<touchgfx::SOUTH> >(pendingPage); break;
    case INSTANT: makeScreen<touchgfx::NoTransition>(pendingPage); break;
    default: makeScreen<CachedWipeTransition<touchgfx::WEST> >(pendingPage); break;
    }
}

void FrontendApplication::startSheet(bool open, int pixels, bool autoplay)
{
    origin = page;
    destination = open ? MENU : menuReturn;
    saveOriginState();
    rollbackQueued = false;
    const FingerSwipeState::Axis axis = open ? FingerSwipeState::SHEET_OPEN : FingerSwipeState::SHEET_CLOSE;
    if (autoplay) swipe.autoplay(open ? 1 : -1, touchgfx::HAL::DISPLAY_HEIGHT, axis);
    else swipe.begin(open ? 1 : -1, pixels, touchgfx::HAL::DISPLAY_HEIGHT, axis);
    fingerTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::beginFingerTransition);
    pendingScreenTransitionCallback = &fingerTransitionCallback;
}

void FrontendApplication::openMenu()
{
    if (page == MENU || swipe.active() || pendingScreenTransitionCallback) return;
    menuReturn = swipeable(page) ? page : MAIN;
    if (touchgfx::HAL::USE_ANIMATION_STORAGE) startSheet(true, 0, true);
    else go(MENU, DOWN);
}

void FrontendApplication::closeMenu()
{
    if (page != MENU || swipe.active() || pendingScreenTransitionCallback) return;
    if (touchgfx::HAL::USE_ANIMATION_STORAGE) startSheet(false, 0, true);
    else go(menuReturn, UP);
}

void FrontendApplication::navigateBack()
{
    if (page == MENU) closeMenu();
    else go(MAIN, BACKWARD);
}

void FrontendApplication::handlePendingScreenTransition()
{
#ifndef SIMULATOR
    const bool measuring=pendingScreenTransitionCallback!=nullptr;
    const uint32_t started=BoardDisplayCycles();
#endif
    if(pendingScreenTransitionCallback)ui::ThemeScreen::get().detach();
    FrontendApplicationBase::handlePendingScreenTransition();
#ifndef SIMULATOR
    if(measuring) BoardDisplayMeasure(2,started);
#endif
}
