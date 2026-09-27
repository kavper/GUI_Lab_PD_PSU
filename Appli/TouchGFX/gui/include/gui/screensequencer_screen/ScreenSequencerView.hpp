#ifndef SCREENSEQUENCERVIEW_HPP
#define SCREENSEQUENCERVIEW_HPP

#include <gui_generated/screensequencer_screen/ScreenSequencerViewBase.hpp>
#include <gui/screensequencer_screen/ScreenSequencerPresenter.hpp>
#include <stdint.h>

class ScreenSequencerView : public ScreenSequencerViewBase
{
public:
    ScreenSequencerView();
    virtual ~ScreenSequencerView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void seqRun();
    virtual void seqPause();
    virtual void seqStop();
    virtual void seqAdd();
    virtual void seqRemove();
    virtual void seqPrev();
    virtual void seqNext();
protected:
    uint8_t divider;
    void refresh();
};

#endif
