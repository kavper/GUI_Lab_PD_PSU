#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP
#include <gui_generated/common/FrontendApplicationBase.hpp>
#include <gui/common/FingerSwipeState.hpp>
class FrontendHeap;
using namespace touchgfx;
class FrontendApplication : public FrontendApplicationBase
{
public:
    enum SwipePage { OTHER, MAIN, SETTINGS, USB_PD };
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }
    virtual void handleTickEvent();
    virtual void handlePendingScreenTransition();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleDragEvent(const touchgfx::DragEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
    bool isScreenTransitionActive() const
    {
        return swipe.active() || (currentTransition && !currentTransition->isDone());
    }
    void setSwipePage(SwipePage value) { page=value; }
    SwipePage swipePage() const { return page; }
    FingerSwipeState& fingerSwipe() { return swipe; }
private:
    SwipePage page,origin,destination;
    FingerSwipeState swipe;
    int16_t startX,startY;
    bool armed,rollbackQueued;
    MainSwipeEditorState editorState;
    char usbNotice[100];
    uint16_t usbNoticeTicks;
    touchgfx::Callback<FrontendApplication> fingerTransitionCallback;
    void beginFingerTransition();
    void rollbackFingerTransition();
    void saveOriginState();
};
#endif
