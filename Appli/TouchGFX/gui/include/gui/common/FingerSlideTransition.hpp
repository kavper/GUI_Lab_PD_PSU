#ifndef FINGERSLIDETRANSITION_HPP
#define FINGERSLIDETRANSITION_HPP
#include <touchgfx/transitions/Transition.hpp>
#include <touchgfx/widgets/SnapshotWidget.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <gui/common/FingerSwipeState.hpp>
#include <gui/common/DisplayComposition.hpp>

class ScanoutImage : public touchgfx::Image {
public:
    virtual void draw(const touchgfx::Rect& rect) const {
#ifndef SIMULATOR
        if (boardDisplayComposition.active) return;
#endif
        touchgfx::Image::draw(rect);
    }
};
class ScanoutSnapshot : public touchgfx::SnapshotWidget {
public:
    virtual void draw(const touchgfx::Rect& rect) const {
#ifndef SIMULATOR
        if (boardDisplayComposition.active) return;
#endif
        touchgfx::SnapshotWidget::draw(rect);
    }
};

// Reuses the HAL animation storage, which is already allocated in PSRAM.
class FingerSlideTransition : public touchgfx::Transition
{
public:
    FingerSlideTransition();
    virtual void init();
    virtual void handleTickEvent();
    virtual void tearDown();
private:
    ScanoutSnapshot outgoing;
    ScanoutImage incoming;
    touchgfx::BitmapId incomingId;
    touchgfx::Drawable* hidden[128];
    unsigned hiddenCount;
    bool incomingAttached;
    void releaseIncoming();
    FingerSwipeState* swipe;
    bool attached;
};
#endif
