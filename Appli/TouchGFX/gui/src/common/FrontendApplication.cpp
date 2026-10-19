#include <gui/common/UiTheme.hpp>
#include <gui/common/UiSound.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/common/FingerSlideTransition.hpp>
#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/screensettings_screen/ScreenSettingsView.hpp>
#include <gui/screensettings_screen/ScreenSettingsPresenter.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdPresenter.hpp>
#include <touchgfx/transitions/NoTransition.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <stdlib.h>
#ifndef SIMULATOR
#include "board_panel.h"
#include <gui/common/DisplayComposition.hpp>
extern "C" volatile uint32_t BoardGraphicsBenchmark;
extern "C" volatile uint32_t BoardGraphicsSwipeBenchmark;
extern "C" volatile uint32_t BoardGraphicsMotion;
#endif

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m,heap), page(OTHER),origin(OTHER),destination(OTHER),
      startX(0),startY(0),armed(false),rollbackQueued(false),usbNoticeTicks(0)
{
#ifdef SIMULATOR
    static uint32_t bitmapCache[2 * 1024 * 1024 / 4];
    touchgfx::Bitmap::setCache(reinterpret_cast<uint16_t*>(bitmapCache), sizeof(bitmapCache), 516);
#endif
    usbNotice[0]=0;
    touchgfx::HAL::getInstance()->setFrameRateCompensation(true);
}
void FrontendApplication::handleTickEvent()
{
    model.tick();
    swipe.tick();
    FrontendApplicationBase::handleTickEvent();
#ifndef SIMULATOR
    BoardGraphicsMotion = isScreenTransitionActive() ? 1 : 0;
    // Opt-in SWD benchmark; default zero, never changes output/setpoints.
    if (BoardGraphicsSwipeBenchmark && !boardDisplayComposition.resumeFrames && !isScreenTransitionActive() && !pendingScreenTransitionCallback &&
        (page == MAIN || page == SETTINGS)) {
        --BoardGraphicsSwipeBenchmark;
        origin = page; destination = page == MAIN ? SETTINGS : MAIN;
        saveOriginState(); rollbackQueued = false;
        swipe.begin(page == MAIN ? 1 : -1, page == MAIN ? touchgfx::HAL::DISPLAY_WIDTH*2/5 : -touchgfx::HAL::DISPLAY_WIDTH*2/5, touchgfx::HAL::DISPLAY_WIDTH);
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
            for (int n=0; n<abs(panel.delta); ++n) {
                if (panel.delta > 0) view->nudgeVoltUp(); else view->nudgeVoltDown();
            }
        } else if (panel.kind == BOARD_CURRENT) {
            for (int n=0; n<abs(panel.delta); ++n) {
                if (panel.delta > 0) view->nudgeAmpUp(); else view->nudgeAmpDown();
            }
        } else if (panel.kind == BOARD_VOLTAGE_STEP) { view->selectVoltage(); view->stepVolt(); }
        else if (panel.kind == BOARD_CURRENT_STEP) { view->selectCurrent(); view->stepAmp(); }
    }
#endif
    ui::ThemeScreen::get().sync();
    if(swipe.phase==FingerSwipeState::FINISHED && !rollbackQueued)
    {
        if(swipe.committed()) { swipe.reset(); armed=false; }
        else
        {
            rollbackQueued=true;
            fingerTransitionCallback=touchgfx::Callback<FrontendApplication>(this,&FrontendApplication::rollbackFingerTransition);
            pendingScreenTransitionCallback=&fingerTransitionCallback;
        }
    }
}
void FrontendApplication::handleClickEvent(const touchgfx::ClickEvent& event)
{
    if(event.getType()==touchgfx::ClickEvent::PRESSED)
        edgeBack.begin(event.getX(),event.getY(),!isScreenTransitionActive()&&!pendingScreenTransitionCallback&&(page==DETAIL||page==BATTERY||page==BMS||(page==USB_PD&&backPage!=MAIN)));
    else if(event.getType()==touchgfx::ClickEvent::RELEASED && edgeBack.active) {
        edgeBack.move(event.getX(),event.getY());
        if(edgeBack.finish()) {
            FrontendApplicationBase::handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::CANCEL,event.getX(),event.getY()));
            navigateBack();
            armed=false;return;
        }
    } else if(event.getType()==touchgfx::ClickEvent::CANCEL)edgeBack.active=false;
    if(swipe.active())
    {
        if(event.getType()==touchgfx::ClickEvent::RELEASED)
        {
            swipe.move(event.getX()-startX); swipe.release(); armed=false;
        }
        else if(event.getType()==touchgfx::ClickEvent::CANCEL)
        { swipe.release(true); armed=false; }
        return;
    }
    if(event.getType()==touchgfx::ClickEvent::PRESSED)
    {
        if (!isScreenTransitionActive() && !pendingScreenTransitionCallback) ui::Sound::touch();
        armed=page!=OTHER && !isScreenTransitionActive() && !pendingScreenTransitionCallback;
        startX=event.getX(); startY=event.getY();
        if (armed && page == MAIN) static_cast<Screen1View*>(currentScreen)->saveSwipeEditor(editorState);
    }
    FrontendApplicationBase::handleClickEvent(event);
    if(event.getType()!=touchgfx::ClickEvent::PRESSED) armed=false;
}
void FrontendApplication::handleDragEvent(const touchgfx::DragEvent& event)
{
    const bool wasCancelled=edgeBack.cancelled;
    if(edgeBack.move(event.getNewX(),event.getNewY())) {
        if(!wasCancelled)FrontendApplicationBase::handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::CANCEL,startX,startY));
        return;
    }
    if(swipe.active()) { swipe.move(event.getNewX()-startX); return; }
    const int dx=event.getNewX()-startX,dy=event.getNewY()-startY;
    const int direction=dx>0?1:-1;
    const bool route=page==MAIN || (page==SETTINGS && direction<0) || (page==USB_PD && backPage==MAIN && direction>0);
    if(armed && route && abs(dx)>=12 && abs(dx)>2*abs(dy) && touchgfx::HAL::USE_ANIMATION_STORAGE)
    {
        // Cancel the pressed button before the view is destroyed.
        FrontendApplicationBase::handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::CANCEL,startX,startY));
        origin=page;
        destination=page==MAIN ? (direction>0?SETTINGS:USB_PD) : MAIN;
        if (origin != MAIN) saveOriginState();
        rollbackQueued=false;
        swipe.begin(direction,dx,touchgfx::HAL::DISPLAY_WIDTH);
        fingerTransitionCallback=touchgfx::Callback<FrontendApplication>(this,&FrontendApplication::beginFingerTransition);
        pendingScreenTransitionCallback=&fingerTransitionCallback;
        return;
    }
    FrontendApplicationBase::handleDragEvent(event);
}
void FrontendApplication::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    if(swipe.active())
    {
        if(event.getType()==touchgfx::GestureEvent::SWIPE_HORIZONTAL) swipe.flick(event.getVelocity());
        return;
    }
    FrontendApplicationBase::handleGestureEvent(event);
}
void FrontendApplication::saveOriginState()
{
    if(origin==MAIN) static_cast<Screen1View*>(currentScreen)->saveSwipeEditor(editorState);
    else if(origin==USB_PD) static_cast<ScreenUsbPdView*>(currentScreen)->saveSwipeNotice(usbNotice,usbNoticeTicks);
}
void FrontendApplication::beginFingerTransition()
{
    backPage=MAIN;
    if(destination==SETTINGS)
        touchgfx::makeTransition<ScreenSettingsView,ScreenSettingsPresenter,FingerSlideTransition,Model>(&currentScreen,&currentPresenter,frontendHeap,&currentTransition,&model);
    else if(destination==USB_PD)
        touchgfx::makeTransition<ScreenUsbPdView,ScreenUsbPdPresenter,FingerSlideTransition,Model>(&currentScreen,&currentPresenter,frontendHeap,&currentTransition,&model);
    else
        touchgfx::makeTransition<Screen1View,Screen1Presenter,FingerSlideTransition,Model>(&currentScreen,&currentPresenter,frontendHeap,&currentTransition,&model);
}
void FrontendApplication::navigateBack()
{
    if(backPage==MAIN)gotoScreen1ScreenWipeTransitionEast();
    else if(backPage==BATTERY)gotoScreenBatteryScreenWipeTransitionEast();
    else gotoScreenSettingsScreenWipeTransitionEast();
}
void FrontendApplication::rollbackFingerTransition()
{
    if(origin==MAIN)
    {
        touchgfx::makeTransition<Screen1View,Screen1Presenter,touchgfx::NoTransition,Model>(&currentScreen,&currentPresenter,frontendHeap,&currentTransition,&model);
        static_cast<Screen1View*>(currentScreen)->restoreSwipeEditor(editorState);
    }
    else if(origin==SETTINGS)
        touchgfx::makeTransition<ScreenSettingsView,ScreenSettingsPresenter,touchgfx::NoTransition,Model>(&currentScreen,&currentPresenter,frontendHeap,&currentTransition,&model);
    else
    {
        touchgfx::makeTransition<ScreenUsbPdView,ScreenUsbPdPresenter,touchgfx::NoTransition,Model>(&currentScreen,&currentPresenter,frontendHeap,&currentTransition,&model);
        static_cast<ScreenUsbPdView*>(currentScreen)->restoreSwipeNotice(usbNotice,usbNoticeTicks);
    }
    swipe.reset(); armed=false; rollbackQueued=false;
}

void FrontendApplication::handlePendingScreenTransition()
{
    if(pendingScreenTransitionCallback)ui::ThemeScreen::get().detach();
    FrontendApplicationBase::handlePendingScreenTransition();
}
