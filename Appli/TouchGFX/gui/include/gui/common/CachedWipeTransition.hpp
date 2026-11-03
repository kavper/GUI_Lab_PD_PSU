#ifndef CACHEDWIPETRANSITION_HPP
#define CACHEDWIPETRANSITION_HPP
#include <touchgfx/transitions/WipeTransition.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/FingerSlideTransition.hpp>
#include <gui/common/KitLayout.hpp>
#include <gui/common/FrontendApplication.hpp>

// Keep the Designer wipe direction/easing, but draw the destination once.
// Every animation tick then copies only the newly exposed bitmap strip.
template <touchgfx::Direction direction>
class CachedWipeTransition : public touchgfx::WipeTransition<direction>
{
public:
    CachedWipeTransition() : touchgfx::WipeTransition<direction>(30), id(touchgfx::BITMAP_INVALID), count(0), attached(false), step(0), finishPending(false) {
        outgoing.setPosition(0,0,touchgfx::HAL::DISPLAY_WIDTH,touchgfx::HAL::DISPLAY_HEIGHT);
#ifdef SIMULATOR
        outgoing.makeSnapshot();
#else
        capturedOutgoing=touchgfx::HAL::getInstance()->getTFTFrameBuffer();
#endif
    }
    virtual void init()
    {
#ifndef SIMULATOR
        const uint32_t initStarted=BoardDisplayCycles();
#endif
        touchgfx::WipeTransition<direction>::init();
        kit::applyDesignerLayout(static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->swipePage(), *this->screenContainer);
        ui::ThemeScreen::get().sync();
        id = touchgfx::Bitmap::dynamicBitmapCreate(touchgfx::HAL::DISPLAY_WIDTH,
                touchgfx::HAL::DISPLAY_HEIGHT, touchgfx::Bitmap::RGB565);
        if (id == touchgfx::BITMAP_INVALID) return;
#ifndef SIMULATOR
        capture.begin(*this->screenContainer);
        boardDisplayComposition={true,0,0,touchgfx::Rect(),0,true};
        capture.advance(*this->screenContainer,id);
        BoardDisplayMeasure(3,initStarted);return;
#else
        ui::ThemeScreen::get().capture(*this->screenContainer,id);
        attachIncoming();
#endif
    }
    void attachIncoming()
    {
        for (touchgfx::Drawable* child = this->screenContainer->getFirstChild(); child; child = child->getNextSibling())
        {
            if (child->isVisible()) {
                if (count == 128) { release(); return; }
                hidden[count++] = child; child->setVisible(false);
            }
        }
        image.setBitmap(touchgfx::Bitmap(id));
        this->screenContainer->add(image); attached = true;
#ifndef SIMULATOR
        BoardDisplayPrepare(capturedOutgoing,
                            reinterpret_cast<const uint16_t*>(touchgfx::Bitmap::dynamicBitmapGetAddress(id)));
        boardDisplayComposition = {true,0,0,touchgfx::Rect()};
#endif
    }
    virtual void tearDown()
    {
        release();
        touchgfx::WipeTransition<direction>::tearDown();
        this->screenContainer->invalidate();
    }
    virtual void handleTickEvent()
    {
#ifndef SIMULATOR
        if(boardDisplayComposition.preparing){
            if(!capture.advance(*this->screenContainer,id))return;
            attachIncoming();
        }
        if(finishPending) {
            if(!BoardDisplayReady()) { this->screenContainer->invalidate(); return; }
            const bool restored=BoardDisplayFinish();
            release(); this->done=true;
            if(restored) touchgfx::Application::getInstance()->clearCachedAreas();
            else this->screenContainer->invalidate();
            return;
        }
#endif
        touchgfx::WipeTransition<direction>::handleTickEvent();
#ifndef SIMULATOR
        const int width=touchgfx::HAL::DISPLAY_WIDTH, height=touchgfx::HAL::DISPLAY_HEIGHT;
        const bool horizontal = direction==touchgfx::EAST || direction==touchgfx::WEST;
        const int span = horizontal ? width : height;
        int visible=touchgfx::EasingEquations::cubicEaseOut(++step,0,span,30);
        if (visible&1) ++visible; // Same aligned two-pixel boundary as WipeTransition.
        if (visible>span) visible=span;
        boardDisplayComposition.incomingClip =
            direction==touchgfx::EAST ? touchgfx::Rect(width-visible,0,visible,height) :
            direction==touchgfx::WEST ? touchgfx::Rect(0,0,visible,height) :
            direction==touchgfx::SOUTH ? touchgfx::Rect(0,height-visible,width,visible) :
                                        touchgfx::Rect(0,0,width,visible);
        if(boardDisplayComposition.active && (step>=30 || this->isDone())) {
            this->screenContainer->invalidate();
            if(this->isDone()) { this->done=false; finishPending=true; }
        }
#endif
        // Completed transitions remain allocated until the next navigation.
        // Restore live controls now so buttons are immediately touchable.
        if (this->isDone() && attached) { release(); this->screenContainer->invalidate(); }
    }
    virtual void invalidate()
    {
        if (this->isDone()) this->screenContainer->invalidate();
        else touchgfx::WipeTransition<direction>::invalidate();
    }
private:
    ScanoutImage image;
    ScanoutSnapshot outgoing;
    touchgfx::BitmapId id;
    touchgfx::Drawable* hidden[128];
    unsigned count;
    bool attached;
    unsigned step;
    bool finishPending;
#ifndef SIMULATOR
    const uint16_t* capturedOutgoing;
    BoundedSnapshotCapture capture;
#endif
    void release()
    {
#ifndef SIMULATOR
        if(this->screenContainer)capture.detach(*this->screenContainer);
        BoardDisplayFinish();
#endif
        if (attached) this->screenContainer->remove(image);
        attached = false;
        while (count) hidden[--count]->setVisible(true);
        if (id != touchgfx::BITMAP_INVALID) touchgfx::Bitmap::dynamicBitmapDelete(id);
        id = touchgfx::BITMAP_INVALID;
    }
};
#endif
