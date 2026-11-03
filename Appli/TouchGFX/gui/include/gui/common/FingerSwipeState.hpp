#ifndef FINGERSWIPESTATE_HPP
#define FINGERSWIPESTATE_HPP
#include <stdint.h>

// Independent of rendering: one pixel of finger travel is one pixel of screen
// travel. A reversed/short drag settles back to the original screen.
//  HORIZONTAL  - both pages slide sideways (page swipe),
//  SHEET_OPEN  - the menu sheet slides down over the still page,
//  SHEET_CLOSE - the menu sheet slides up and uncovers the page beneath.
class FingerSwipeState
{
public:
    enum Phase { IDLE, DRAGGING, SETTLING, FINISHED };
    enum Axis { HORIZONTAL, SHEET_OPEN, SHEET_CLOSE };
    FingerSwipeState() { reset(); }
    void reset() { phase=IDLE; axis=HORIZONTAL; direction=0; position=0; width=800; velocity=0; step=0; steps=20; from=to=0; forcedCancel=false; }
    bool active() const { return phase != IDLE; }
    bool vertical() const { return axis != HORIZONTAL; }
    void begin(int sign, int pixels, int span, Axis how=HORIZONTAL)
    {
        reset(); phase=DRAGGING; axis=how; direction=sign; width=span; move(pixels);
    }
    // Button driven: run the same motion without a finger, start to end.
    void autoplay(int sign, int span, Axis how)
    {
        reset(); axis=how; direction=sign; width=span; phase=SETTLING; step=0; steps=22; from=0; to=sign*span;
    }
    void move(int pixels)
    {
        if (phase != DRAGGING) return;
        int amount=direction*pixels;
        if (amount<0) amount=0;
        if (amount>width) amount=width;
        position=direction*amount;
    }
    void flick(int speed)
    {
        velocity=speed;
        // Some touch controllers report the velocity after RELEASED.
        if (phase==SETTLING && step==0 && !forcedCancel) chooseTarget(false);
    }
    void release(bool cancelled=false)
    {
        if (phase!=DRAGGING) return;
        forcedCancel=cancelled; from=position; phase=SETTLING; step=0; chooseTarget(cancelled);
    }
    void tick()
    {
        if (phase!=SETTLING) return;
        ++step;
        // Integer cubic ease-out; exact endpoints and no floating-point work.
        const int remaining=steps-step;
        const int denominator=steps*steps*steps;
        position=to+(from-to)*remaining*remaining*remaining/denominator;
        if (step>=steps) { position=to; phase=FINISHED; }
    }
    bool committed() const { return to != 0; }
    Phase phase;
    Axis axis;
    int direction,position,width;
private:
    void chooseTarget(bool cancelled)
    {
        const int amount=direction*position;
        const bool quick=direction*velocity>=4 && amount>=width/20;
        to=!cancelled && (amount>=width/4 || quick) ? direction*width : 0;
        from=position;
        // Shorter settle for the remaining distance keeps the speed of the finger.
        const int left=(to>from?to-from:from-to);
        steps=8+12*left/(width?width:1);
    }
    int velocity,step,steps,from,to;
    bool forcedCancel;
};

struct MainSwipeEditorState
{
    uint8_t target,length,preset;
    bool replace;
    char text[12];
    uint16_t status[40];
    uint32_t statusColor;
    uint16_t statusTicks;
};
#endif
