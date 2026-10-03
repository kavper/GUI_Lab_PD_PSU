#ifndef FINGERSWIPESTATE_HPP
#define FINGERSWIPESTATE_HPP
#include <stdint.h>

// Independent of rendering: one pixel of horizontal finger travel is one pixel
// of screen travel. A reversed/short drag settles back to the original screen.
class FingerSwipeState
{
public:
    enum Phase { IDLE, DRAGGING, SETTLING, FINISHED };
    FingerSwipeState() { reset(); }
    void reset() { phase=IDLE; direction=0; position=0; width=800; velocity=0; step=0; steps=14; from=to=0; forcedCancel=false; }
    bool active() const { return phase != IDLE; }
    void begin(int sign, int pixels, int screenWidth)
    {
        reset(); phase=DRAGGING; direction=sign; width=screenWidth; move(pixels);
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
    int direction,position,width;
private:
    void chooseTarget(bool cancelled)
    {
        const int amount=direction*position;
        const bool quick=direction*velocity>=4 && amount>=width/20;
        to=!cancelled && (amount>=width/4 || quick) ? direction*width : 0;
        from=position;
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
};
#endif
