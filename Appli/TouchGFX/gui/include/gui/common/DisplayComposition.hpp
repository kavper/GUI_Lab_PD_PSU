#ifndef DISPLAYCOMPOSITION_HPP
#define DISPLAYCOMPOSITION_HPP
#include <touchgfx/hal/Types.hpp>
#ifndef SIMULATOR
// GUI-task-only composition state. Sources are pre-rotated once, then copied
// directly into scanout; logical framebuffers are rebuilt after the transition.
struct DisplayComposition {
    bool active;
    int outgoingX, incomingX;
    touchgfx::Rect incomingClip;
    unsigned resumeFrames;
};
extern DisplayComposition boardDisplayComposition;
void BoardDisplayPrepare(const uint16_t* outgoing, const uint16_t* incoming);
inline void BoardDisplayFinish()
{
    if (boardDisplayComposition.active) {
        boardDisplayComposition.active=false;
        boardDisplayComposition.resumeFrames=2;
    }
}
#endif
#endif
