#include <gui/common/FingerSlideTransition.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/UiTheme.hpp>
#include <touchgfx/hal/HAL.hpp>

FingerSlideTransition::FingerSlideTransition()
    : incomingId(touchgfx::BITMAP_INVALID), hiddenCount(0), incomingAttached(false),
      swipe(&static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->fingerSwipe()), attached(false)
{
    outgoing.setPosition(0,0,touchgfx::HAL::DISPLAY_WIDTH,touchgfx::HAL::DISPLAY_HEIGHT);
    outgoing.makeSnapshot();
}
void FingerSlideTransition::init()
{
    // Render the destination once. Sliding two opaque images avoids redrawing
    // text and hundreds of themed corner strips on every animation frame.
    ui::ThemeScreen::get().sync();
    incomingId = touchgfx::Bitmap::dynamicBitmapCreate(swipe->width,
                    touchgfx::HAL::DISPLAY_HEIGHT, touchgfx::Bitmap::RGB565);
    if (incomingId != touchgfx::BITMAP_INVALID)
    {
        screenContainer->drawToDynamicBitmap(incomingId);
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
    touchgfx::Drawable* widget=screenContainer->getFirstChild();
    while(widget)
    {
        widget->setX(widget->getX()-swipe->direction*swipe->width);
        widget=widget->getNextSibling();
    }
    screenContainer->add(outgoing); attached=true;
#ifndef SIMULATOR
    if (incomingAttached) {
        BoardDisplayPrepare(reinterpret_cast<const uint16_t*>(touchgfx::HAL::getInstance()->getAnimationStorage()),
                            reinterpret_cast<const uint16_t*>(touchgfx::Bitmap::dynamicBitmapGetAddress(incomingId)));
        boardDisplayComposition = {true, 0, -swipe->direction*swipe->width,
            touchgfx::Rect(0,0,swipe->width,touchgfx::HAL::DISPLAY_HEIGHT)};
    }
#endif
}
void FingerSlideTransition::handleTickEvent()
{
    // RGB565: move on a two-pixel boundary for aligned accelerated blits.
    const int position=swipe->position-(swipe->position%2);
    const int delta=position-outgoing.getX();
#ifndef SIMULATOR
    if (boardDisplayComposition.active) {
        boardDisplayComposition.outgoingX = position;
        boardDisplayComposition.incomingX = position - swipe->direction*swipe->width;
    }
#endif
    if(delta)
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
        // On cancellation keep the original snapshot covering the destination
        // until the original view is reconstructed in the next transition.
        if(swipe->committed()) { screenContainer->remove(outgoing); attached=false; releaseIncoming(); screenContainer->invalidate(); }
        done=true;
    }
}
void FingerSlideTransition::tearDown()
{
    if(screenContainer && attached) { screenContainer->remove(outgoing); attached=false; }
    releaseIncoming();
}

void FingerSlideTransition::releaseIncoming()
{
#ifndef SIMULATOR
    BoardDisplayFinish();
#endif
    if (screenContainer && incomingAttached) screenContainer->remove(incoming);
    incomingAttached = false;
    while (hiddenCount) hidden[--hiddenCount]->setVisible(true);
    if (incomingId != touchgfx::BITMAP_INVALID) touchgfx::Bitmap::dynamicBitmapDelete(incomingId);
    incomingId = touchgfx::BITMAP_INVALID;
}
