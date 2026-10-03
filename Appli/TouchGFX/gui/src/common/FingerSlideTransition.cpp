#include <gui/common/FingerSlideTransition.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/hal/HAL.hpp>

FingerSlideTransition::FingerSlideTransition()
    : swipe(&static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->fingerSwipe()), attached(false)
{
    outgoing.setPosition(0,0,touchgfx::HAL::DISPLAY_WIDTH,touchgfx::HAL::DISPLAY_HEIGHT);
    outgoing.makeSnapshot();
}
void FingerSlideTransition::init()
{
    touchgfx::Drawable* widget=screenContainer->getFirstChild();
    while(widget)
    {
        widget->setX(widget->getX()-swipe->direction*swipe->width);
        widget=widget->getNextSibling();
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
        if(swipe->committed()) { screenContainer->remove(outgoing); attached=false; }
        done=true;
    }
}
void FingerSlideTransition::tearDown()
{
    if(screenContainer && attached) { screenContainer->remove(outgoing); attached=false; }
}
