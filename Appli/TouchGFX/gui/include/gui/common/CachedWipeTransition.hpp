#ifndef CACHEDWIPETRANSITION_HPP
#define CACHEDWIPETRANSITION_HPP
#include <touchgfx/transitions/WipeTransition.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/FingerSlideTransition.hpp>

// Keep the Designer wipe direction/easing, but draw the destination once.
// Every animation tick then copies only the newly exposed bitmap strip.
template <touchgfx::Direction direction>
class CachedWipeTransition : public touchgfx::WipeTransition<direction>
{
public:
    CachedWipeTransition() : touchgfx::WipeTransition<direction>(30), id(touchgfx::BITMAP_INVALID), count(0), attached(false), step(0) {
        outgoing.setPosition(0,0,touchgfx::HAL::DISPLAY_WIDTH,touchgfx::HAL::DISPLAY_HEIGHT);
        outgoing.makeSnapshot();
    }
    virtual void init()
    {
        touchgfx::WipeTransition<direction>::init();
        ui::ThemeScreen::get().sync();
        id = touchgfx::Bitmap::dynamicBitmapCreate(touchgfx::HAL::DISPLAY_WIDTH,
                touchgfx::HAL::DISPLAY_HEIGHT, touchgfx::Bitmap::RGB565);
        if (id == touchgfx::BITMAP_INVALID) return;
        this->screenContainer->drawToDynamicBitmap(id);
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
        BoardDisplayPrepare(reinterpret_cast<const uint16_t*>(touchgfx::HAL::getInstance()->getAnimationStorage()),
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
        touchgfx::WipeTransition<direction>::handleTickEvent();
#ifndef SIMULATOR
        const int width=touchgfx::HAL::DISPLAY_WIDTH, height=touchgfx::HAL::DISPLAY_HEIGHT;
        int visible=touchgfx::EasingEquations::cubicEaseOut(++step,0,width,30);
        if (visible&1) ++visible; // Same aligned two-pixel boundary as WipeTransition.
        if (visible>width) visible=width;
        boardDisplayComposition.incomingClip = direction==touchgfx::EAST
            ? touchgfx::Rect(width-visible,0,visible,height) : touchgfx::Rect(0,0,visible,height);
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
    void release()
    {
#ifndef SIMULATOR
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
