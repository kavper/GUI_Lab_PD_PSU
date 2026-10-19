#ifndef LABSWIPE_HPP
#define LABSWIPE_HPP

#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/events/GestureEvent.hpp>
extern "C" {
#include "psu_swipe.h"
}

/* Right-edge return. blocked is set when the press starts on a slider,
   editor, keypad, or list, so that drag never navigates. */
class LabSwipe
{
public:
    LabSwipe() : armed(false), blocked(false), startX(0), startY(0) {}

    void onClick(const touchgfx::ClickEvent& event, bool block)
    {
        if (event.getType() == touchgfx::ClickEvent::PRESSED)
        {
            armed = true;
            blocked = block;
            startX = event.getX();
            startY = event.getY();
        }
        else if (event.getType() == touchgfx::ClickEvent::CANCEL)
        {
            armed = false;
        }
    }

    bool onGesture(const touchgfx::GestureEvent& event)
    {
        const int horizontal =
            event.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL ? 1 : 0;
        const int go = psu_swipe_back(armed ? 1 : 0, startX, startY,
                                       event.getX(), event.getY(),
                                       horizontal, event.getVelocity(),
                                       blocked ? 1 : 0);
        armed = false;
        return go != 0;
    }

private:
    bool armed;
    bool blocked;
    int16_t startX;
    int16_t startY;
};

#endif
