#include <gui/common/FingerSlideTransition.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/Bitmap.hpp>
#include <touchgfx/hal/HAL.hpp>

namespace
{
// A second full-screen RGB565 image lets NeoChrom move two flat surfaces
// instead of rebuilding the complete destination widget tree on every tick.
LOCATION_PRAGMA_NOLOAD("TouchGFX_Framebuffer")
uint32_t incomingScreenBuffer[(800 * 480 * 2 + 3) / 4]
    LOCATION_ATTRIBUTE_NOLOAD("TouchGFX_Framebuffer");
}

FingerSlideTransition::FingerSlideTransition()
    : swipe(&static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->fingerSwipe()),
      incomingBitmap(touchgfx::BITMAP_INVALID), attached(false), cachedDestination(false)
{
    outgoing.setPosition(0,0,touchgfx::HAL::DISPLAY_WIDTH,touchgfx::HAL::DISPLAY_HEIGHT);
    outgoing.makeSnapshot();
}
void FingerSlideTransition::init()
{
    touchgfx::Transition::init();

    incomingBitmap=touchgfx::Bitmap::dynamicBitmapCreateExternal(
        touchgfx::HAL::DISPLAY_WIDTH,
        touchgfx::HAL::DISPLAY_HEIGHT,
        incomingScreenBuffer,
        touchgfx::Bitmap::RGB565);

    if(incomingBitmap!=touchgfx::BITMAP_INVALID)
    {
        // Render the complex destination once. During the gesture only these
        // two opaque RGB565 surfaces are submitted to NeoChrom/DMA2D.
        screenContainer->drawToDynamicBitmap(incomingBitmap);
        incoming.setPosition(-swipe->direction*swipe->width,0,
                             touchgfx::HAL::DISPLAY_WIDTH,touchgfx::HAL::DISPLAY_HEIGHT);
        incoming.setBitmapFormat(touchgfx::Bitmap::RGB565);
        incoming.setPixelData(reinterpret_cast<uint8_t*>(incomingScreenBuffer));
        screenContainer->add(incoming);
        cachedDestination=true;
    }
    else
    {
        // Safe fallback if the dynamic bitmap descriptor cannot be reserved.
        touchgfx::Drawable* widget=screenContainer->getFirstChild();
        while(widget)
        {
            widget->setX(widget->getX()-swipe->direction*swipe->width);
            widget=widget->getNextSibling();
        }
    }
    screenContainer->add(outgoing); attached=true;
}
void FingerSlideTransition::handleTickEvent()
{
    // RGB565: move on a two-pixel boundary for aligned accelerated blits.
    const int position=swipe->position-(swipe->position%2);
    const int delta=position-outgoing.getX();
    if(delta)
    {
        if(cachedDestination)
        {
            outgoing.moveRelative(delta,0);
            incoming.moveRelative(delta,0);
        }
        else
        {
            touchgfx::Drawable* widget=screenContainer->getFirstChild();
            while(widget)
            {
                widget->setX(widget->getX()+delta);
                widget=widget->getNextSibling();
            }
        }
        screenContainer->invalidate();
    }
    if(swipe->phase==FingerSwipeState::FINISHED)
    {
        // On cancellation keep the original snapshot covering the destination
        // until the original view is reconstructed in the next transition.
        if(swipe->committed())
        {
            screenContainer->remove(outgoing);
            if(cachedDestination) screenContainer->remove(incoming);
            attached=false;
        }
        done=true;
    }
}
void FingerSlideTransition::tearDown()
{
    if(screenContainer && attached)
    {
        screenContainer->remove(outgoing);
        if(cachedDestination) screenContainer->remove(incoming);
        attached=false;
    }
    if(incomingBitmap!=touchgfx::BITMAP_INVALID)
    {
        touchgfx::Bitmap::dynamicBitmapDelete(incomingBitmap);
        incomingBitmap=touchgfx::BITMAP_INVALID;
    }
    cachedDestination=false;
}
