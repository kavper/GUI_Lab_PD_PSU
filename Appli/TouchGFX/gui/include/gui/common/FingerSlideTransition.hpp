#ifndef FINGERSLIDETRANSITION_HPP
#define FINGERSLIDETRANSITION_HPP
#include <touchgfx/transitions/Transition.hpp>
#include <touchgfx/widgets/SnapshotWidget.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <gui/common/FingerSwipeState.hpp>
#include <gui/common/DisplayComposition.hpp>
#include <gui/common/UiTheme.hpp>

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
class SnapshotCaptureCover : public touchgfx::Widget {
public:
    virtual touchgfx::Rect getSolidRect() const { return touchgfx::Rect(0,0,getWidth(),getHeight()); }
    virtual void draw(const touchgfx::Rect&) const {}
};
#ifndef SIMULATOR
class BoundedSnapshotCapture {
public:
    BoundedSnapshotCapture():step(0),attached(false){}
    void begin(touchgfx::Container& container){
        cover.setPosition(0,0,container.getWidth(),container.getHeight());
        container.add(cover);attached=true;step=0;
    }
    bool advance(touchgfx::Container& container,touchgfx::BitmapId bitmap){
        if(step<4){
            const int height=container.getHeight(),y=step*height/4;
            const uint32_t started=BoardDisplayCycles();
            ui::ThemeScreen::get().capture(container,bitmap,touchgfx::Rect(0,y,container.getWidth(),(step+1)*height/4-y));
            BoardDisplayMeasure(0,started);++step;return false;
        }
        detach(container);return true;
    }
    void detach(touchgfx::Container& container){if(attached){container.remove(cover);attached=false;}}
private:
    SnapshotCaptureCover cover;
    unsigned step;
    bool attached;
};
#endif

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
    bool finalPresented;
#ifndef SIMULATOR
    const uint16_t* capturedOutgoing;
    BoundedSnapshotCapture capture;
#endif
    void attachIncoming();
#ifndef SIMULATOR
    void layout();
#endif
};
#endif
