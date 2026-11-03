#include <gui/common/FingerSlideTransition.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/UiTheme.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <gui/common/KitLayout.hpp>

FingerSlideTransition::FingerSlideTransition()
    : incomingId(touchgfx::BITMAP_INVALID), hiddenCount(0), incomingAttached(false),
      swipe(&static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->fingerSwipe()), attached(false), finalPresented(false)
{
    outgoing.setPosition(0,0,touchgfx::HAL::DISPLAY_WIDTH,touchgfx::HAL::DISPLAY_HEIGHT);
#ifdef SIMULATOR
    outgoing.makeSnapshot();
#else
    capturedOutgoing=touchgfx::HAL::getInstance()->getTFTFrameBuffer();
#endif
}
void FingerSlideTransition::init()
{
#ifndef SIMULATOR
    const uint32_t initStarted=BoardDisplayCycles();
#endif
    kit::applyDesignerLayout(static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->swipePage(), *screenContainer);
    // Render the destination once. Sliding two opaque images avoids redrawing
    // text and hundreds of themed corner strips on every animation frame.
    ui::ThemeScreen::get().sync();
    incomingId = touchgfx::Bitmap::dynamicBitmapCreate(touchgfx::HAL::DISPLAY_WIDTH,
                    touchgfx::HAL::DISPLAY_HEIGHT, touchgfx::Bitmap::RGB565);
#ifndef SIMULATOR
    if(incomingId!=touchgfx::BITMAP_INVALID){
        capture.begin(*screenContainer);
        boardDisplayComposition={true,0,0,
            touchgfx::Rect(0,0,touchgfx::HAL::DISPLAY_WIDTH,touchgfx::HAL::DISPLAY_HEIGHT),0,true};
        layout();
        capture.advance(*screenContainer,incomingId);
        BoardDisplayMeasure(3,initStarted);return;
    }
#endif
    if(incomingId!=touchgfx::BITMAP_INVALID)ui::ThemeScreen::get().capture(*screenContainer,incomingId);
    attachIncoming();
}
void FingerSlideTransition::attachIncoming()
{
    if (incomingId != touchgfx::BITMAP_INVALID)
    {
        touchgfx::Drawable* child = screenContainer->getFirstChild();
        while (child)
        {
            if (child->isVisible()) {
                if (hiddenCount == 128) { releaseIncoming(); break; }
                hidden[hiddenCount++] = child; child->setVisible(false);
            }
            child = child->getNextSibling();
        }
        if (incomingId != touchgfx::BITMAP_INVALID) {
            incoming.setBitmap(touchgfx::Bitmap(incomingId));
            incoming.setXY(0, 0);
            screenContainer->add(incoming); incomingAttached = true;
        }
    }
    if (!swipe->vertical()) {
        touchgfx::Drawable* widget=screenContainer->getFirstChild();
        while(widget)
        {
            widget->setX(widget->getX()-swipe->direction*swipe->width);
            widget=widget->getNextSibling();
        }
    }
    screenContainer->add(outgoing); attached=true;
    // Opening sheet: the menu slides over the still page.
    if (incomingAttached && swipe->axis==FingerSwipeState::SHEET_OPEN) { screenContainer->remove(incoming); screenContainer->add(incoming); }
    if (swipe->axis==FingerSwipeState::SHEET_OPEN) incoming.setY(swipe->position-swipe->position%2-touchgfx::HAL::DISPLAY_HEIGHT);
    else if (swipe->axis==FingerSwipeState::SHEET_CLOSE) outgoing.setY(swipe->position-swipe->position%2);
#ifndef SIMULATOR
    if (incomingAttached) {
        BoardDisplayPrepare(capturedOutgoing,
                            reinterpret_cast<const uint16_t*>(touchgfx::Bitmap::dynamicBitmapGetAddress(incomingId)));
        boardDisplayComposition = {true, 0, 0,
            touchgfx::Rect(0,0,touchgfx::HAL::DISPLAY_WIDTH,touchgfx::HAL::DISPLAY_HEIGHT)};
        layout();
    } else outgoing.makeSnapshot(); // Preserve the ordinary rendering fallback.
#endif
}
void FingerSlideTransition::handleTickEvent()
{
#ifndef SIMULATOR
    if(boardDisplayComposition.preparing){
        if(!capture.advance(*screenContainer,incomingId))return;
        attachIncoming();
    }
#endif
    // RGB565: move on a two-pixel boundary for aligned accelerated blits.
    const int position=swipe->position-(swipe->position%2);
    const int delta=position-outgoing.getX();
#ifndef SIMULATOR
    if (boardDisplayComposition.active) layout();
#endif
    if (swipe->vertical()) {
        const int height=touchgfx::HAL::DISPLAY_HEIGHT;
        if (swipe->axis==FingerSwipeState::SHEET_OPEN) { if (incoming.getY()!=position-height) { incoming.setY(position-height); screenContainer->invalidate(); } }
        else if (outgoing.getY()!=position) { outgoing.setY(position); screenContainer->invalidate(); }
    }
    else if(delta)
    {
        touchgfx::Drawable* widget=screenContainer->getFirstChild();
        while(widget)
        {
            widget->setX(widget->getX()+delta);
            widget=widget->getNextSibling();
        }
        screenContainer->invalidate();
    }
    if(swipe->phase==FingerSwipeState::FINISHED)
    {
#ifndef SIMULATOR
        // Present the endpoint into both alternating scanout buffers before
        // exposing the live widgets. Logical buffers were restored in parallel.
        if(swipe->committed() && boardDisplayComposition.active && (!finalPresented || !BoardDisplayReady())) {
            finalPresented=true; screenContainer->invalidate(); return;
        }
#endif
        // On cancellation keep the original snapshot covering the destination
        // until the original view is reconstructed in the next transition.
        if(swipe->committed()) {
            screenContainer->remove(outgoing); attached=false;
#ifndef SIMULATOR
            const bool restored=BoardDisplayFinish();
#else
            const bool restored=false;
#endif
            releaseIncoming();
            if(restored) touchgfx::Application::getInstance()->clearCachedAreas();
            else screenContainer->invalidate();
        }
        done=true;
    }
}
#ifndef SIMULATOR
void FingerSlideTransition::layout()
{
    const int position=swipe->position-(swipe->position%2);
    DisplayComposition& c=boardDisplayComposition;
    c.outgoingX=c.incomingX=c.outgoingY=c.incomingY=0; c.outgoingOnTop=false;
    if (swipe->axis==FingerSwipeState::SHEET_OPEN) c.incomingY=position-touchgfx::HAL::DISPLAY_HEIGHT;
    else if (swipe->axis==FingerSwipeState::SHEET_CLOSE) { c.outgoingY=position; c.outgoingOnTop=true; }
    else { c.outgoingX=position; c.incomingX=position-swipe->direction*swipe->width; }
}
#endif
void FingerSlideTransition::tearDown()
{
    if(screenContainer && attached) { screenContainer->remove(outgoing); attached=false; }
    releaseIncoming();
}

void FingerSlideTransition::releaseIncoming()
{
#ifndef SIMULATOR
    if(screenContainer)capture.detach(*screenContainer);
    BoardDisplayFinish();
#endif
    if (screenContainer && incomingAttached) screenContainer->remove(incoming);
    incomingAttached = false;
    while (hiddenCount) hidden[--hiddenCount]->setVisible(true);
    if (incomingId != touchgfx::BITMAP_INVALID) touchgfx::Bitmap::dynamicBitmapDelete(incomingId);
    incomingId = touchgfx::BITMAP_INVALID;
}
