#ifndef DISPLAYCOMPOSITION_HPP
#define DISPLAYCOMPOSITION_HPP
#include <touchgfx/hal/Types.hpp>
#ifndef SIMULATOR
// GUI-task-only composition state. Snapshot regions are rotated directly into
// scanout; logical buffers are restored from the snapshot during the transition.
struct DisplayComposition {
    bool active;
    int outgoingX, incomingX;
    touchgfx::Rect incomingClip;
    unsigned resumeFrames;
    bool preparing;
    // Vertical offsets (menu sheet). By default the incoming layer is drawn
    // over the outgoing one; outgoingOnTop reverses that (sheet closing).
    int outgoingY, incomingY;
    bool outgoingOnTop;
};
extern DisplayComposition boardDisplayComposition;
void BoardDisplayPrepare(const uint16_t* outgoing, const uint16_t* incoming);
// True means both logical buffers were restored from the captured destination.
bool BoardDisplayFinish();
bool BoardDisplayReady();
uint32_t BoardDisplayCycles();
void BoardDisplayMeasure(unsigned stage,uint32_t started);
#endif
#endif
