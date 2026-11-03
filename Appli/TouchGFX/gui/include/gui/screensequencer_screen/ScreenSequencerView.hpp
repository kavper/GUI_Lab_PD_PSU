#ifndef SCREENSEQUENCERVIEW_HPP
#define SCREENSEQUENCERVIEW_HPP

#include <gui_generated/screensequencer_screen/ScreenSequencerViewBase.hpp>
#include <gui/screensequencer_screen/ScreenSequencerPresenter.hpp>
#include <gui/common/Kit.hpp>
#include <stdint.h>
extern "C" {
#include "psu_seq.h"
#include "psu_app.h"
}

// Sequencer: profile preview on top, the step table below (tap a value to
// edit it), run / repeat controls on the right. While a value is edited the
// run card turns into a keypad.
class ScreenSequencerView : public ScreenSequencerViewBase
{
public:
    ScreenSequencerView();
    virtual ~ScreenSequencerView() {}
    virtual void setupScreen();
    virtual void tearDownScreen() { ScreenSequencerViewBase::tearDownScreen(); }
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void handleClickEvent(const touchgfx::ClickEvent& e);
    virtual void handleDragEvent(const touchgfx::DragEvent& e);

private:
    enum { ROWS = 5, COLS = 5, KEYS = 14, ACTIONS = 5, POINTS = 160 };
    uint8_t divider, first;
    bool cycleEditing, listDragging;
    int dragPixels;
    PsuSeqEdit edit;
    PsuEditor cycleEditor;

    kit::Header header;
    kit::Panel profileCard, stepsCard, runCard, rowMark;
    kit::Label profileCaption, legend, tableHead[COLS], listRange;
    kit::Btn cell[ROWS][COLS];
    kit::Btn action[ACTIONS];
    kit::Plot plot;
    kit::Label runCaption, runTime, runTotal, runInfo, repeatCaption;
    kit::Chip runChip;
    kit::Bar progress;
    kit::Btn mode[3], cycleLess, cycleMore, cycleCount, runBtn, stopBtn;
    kit::Label editCaption, editValue, editHint;
    kit::Btn keys[KEYS];
    touchgfx::Callback<ScreenSequencerView, const kit::Btn&> clicked;

    void layout();
    void setupTheme();
    void refresh();
    void refreshProfile(uint32_t& totalMs, uint32_t& elapsedMs);
    void onClick(const kit::Btn& b);
    void showEditor(bool on);
    bool editing() const { return cycleEditing || edit.field != PSU_SEQ_FIELD_NONE; }
    void select(uint8_t index);
    void move(int direction);
    void duplicate();
    void run();
    void scroll(int delta);
    void keyPressed(int k);
    static uint32_t rampMs(uint32_t fromMv, const PsuSeqStep& s);
};

#endif
