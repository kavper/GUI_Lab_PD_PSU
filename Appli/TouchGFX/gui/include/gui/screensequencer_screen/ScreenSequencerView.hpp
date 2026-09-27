#ifndef SCREENSEQUENCERVIEW_HPP
#define SCREENSEQUENCERVIEW_HPP

#include <gui_generated/screensequencer_screen/ScreenSequencerViewBase.hpp>
#include <gui/screensequencer_screen/ScreenSequencerPresenter.hpp>
#include <stdint.h>
extern "C" {
#include "psu_seq.h"
}

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
    virtual void seqFieldVolt();
    virtual void seqFieldAmp();
    virtual void seqFieldTime();
    virtual void seqFieldSlew();
    virtual void seqPick1();
    virtual void seqPick2();
    virtual void seqPick3();
    virtual void seqPick4();
    virtual void seqPick5();
    virtual void seqPick6();
    virtual void seqKey0();
    virtual void seqKey1();
    virtual void seqKey2();
    virtual void seqKey3();
    virtual void seqKey4();
    virtual void seqKey5();
    virtual void seqKey6();
    virtual void seqKey7();
    virtual void seqKey8();
    virtual void seqKey9();
    virtual void seqKeyDot();
    virtual void seqKeyClr();
    virtual void seqKeyDel();
    virtual void seqKeyApply();
protected:
    uint8_t divider;
    uint8_t visible_start;
    PsuSeqEdit edit;
    void refresh();
    void syncEdit();
    void pickRow(uint8_t row);
    void chooseField(uint8_t field);
    void setKeys(bool on, bool enabled);
};

#endif
