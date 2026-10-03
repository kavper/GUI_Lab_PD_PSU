#ifndef FINGERSLIDETRANSITION_HPP
#define FINGERSLIDETRANSITION_HPP
#include <touchgfx/transitions/Transition.hpp>
#include <touchgfx/widgets/SnapshotWidget.hpp>
#include <gui/common/FingerSwipeState.hpp>

// Reuses the HAL animation storage, which is already allocated in PSRAM.
class FingerSlideTransition : public touchgfx::Transition
{
public:
    FingerSlideTransition();
    virtual void init();
    virtual void handleTickEvent();
    virtual void tearDown();
private:
    touchgfx::SnapshotWidget outgoing;
    FingerSwipeState* swipe;
    bool attached;
};
#endif
