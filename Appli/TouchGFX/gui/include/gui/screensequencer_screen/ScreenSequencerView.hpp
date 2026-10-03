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
    void setupTheme();
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
    virtual void seqSyncKeypad();
    virtual void allOff();
    virtual void handleDragEvent(const touchgfx::DragEvent& e);
protected:
    static const int16_t KEYPAD_X = 524;
    static const int16_t KEYPAD_SHOWN_Y = 116;
    static const int16_t KEYPAD_HIDDEN_Y = 132;
    static const uint8_t KEYPAD_TICKS = 12;
    int dragPixels = 0;
    bool listDragging = false;
    virtual void handleClickEvent(const touchgfx::ClickEvent& e);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& e);
    uint8_t divider;
    uint8_t visible_start;
    PsuSeqEdit edit;
    int16_t keypadFromY;
    int16_t keypadToY;
    int16_t keypadFromA;
    int16_t keypadToA;
    int16_t keypadAlpha;
    uint8_t keypadTick;
    bool keypadMoving;
    void refresh();
    void syncEdit();
    void pickRow(uint8_t row);
    void chooseField(uint8_t field);
    void setKeys(bool on, bool enabled);
    void poseKeypad(int16_t y, int16_t alpha);
    void seqStepKeypad();
};

#endif
