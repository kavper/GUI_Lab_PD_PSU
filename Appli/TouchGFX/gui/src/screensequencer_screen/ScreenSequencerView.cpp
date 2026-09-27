#include <gui/screensequencer_screen/ScreenSequencerView.hpp>
#include <gui/common/LabText.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Bitmap.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_format.h"
}

ScreenSequencerView::ScreenSequencerView()
    : divider(0),
      visible_start(0),
      keypadFromY(KEYPAD_HIDDEN_Y),
      keypadToY(KEYPAD_HIDDEN_Y),
      keypadFromA(0),
      keypadToA(0),
      keypadAlpha(0),
      keypadTick(0),
      keypadMoving(false)
{
    psu_seq_edit_init(&edit);
}

void ScreenSequencerView::setupScreen()
{
    ScreenSequencerViewBase::setupScreen();
    keypadMoving = false;
    keypadTick = 0;
    keypadFromY = keypadToY = KEYPAD_HIDDEN_Y;
    keypadFromA = keypadToA = 0;
    SeqKeypad.setVisible(false);
    poseKeypad(KEYPAD_HIDDEN_Y, 0);
    refresh();
}

void ScreenSequencerView::tearDownScreen()
{
    ScreenSequencerViewBase::tearDownScreen();
}

static int16_t easeOut(int16_t t, int16_t b, int16_t c, int16_t d)
{
    int32_t tn = (int32_t)t * 256 / d - 256;
    int32_t cube = tn * tn / 256 * tn / 256;
    return (int16_t)(b + (c * (cube + 256)) / 256);
}

static int16_t easeIn(int16_t t, int16_t b, int16_t c, int16_t d)
{
    int32_t tn = (int32_t)t * 256 / d;
    int32_t cube = tn * tn / 256 * tn / 256;
    return (int16_t)(b + (c * cube) / 256);
}

void ScreenSequencerView::handleTickEvent()
{
    seqStepKeypad();
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

namespace
{
template <typename Area>
void show_step(Area& no, touchgfx::Unicode::UnicodeChar* no_b, uint16_t no_n,
               Area& volt, touchgfx::Unicode::UnicodeChar* volt_b, uint16_t volt_n,
               Area& amp, touchgfx::Unicode::UnicodeChar* amp_b, uint16_t amp_n,
               Area& time, touchgfx::Unicode::UnicodeChar* time_b, uint16_t time_n,
               uint8_t index, const PsuSeqStep* step, int selected)
{
    char v[16];
    char a[16];
    char t[16];
    char id[8];
    touchgfx::colortype value = selected ? lab_cyan() : lab_text();
    touchgfx::colortype dim = lab_muted();
    if (step == 0)
    {
        lab_show(no, no_b, no_n, "--", dim);
        lab_show(volt, volt_b, volt_n, "--", dim);
        lab_show(amp, amp_b, amp_n, "--", dim);
        lab_show(time, time_b, time_n, "--", dim);
        return;
    }
    if (!step->enabled)
        value = dim;
    (void)snprintf(id, sizeof(id), "%02u", (unsigned)index);
    psu_format_voltage(v, sizeof(v), step->voltage_mv);
    psu_format_current_ma(a, sizeof(a), step->current_ma);
    lab_ms(t, sizeof(t), step->time_ms);
    lab_show(no, no_b, no_n, id, step->enabled ? lab_cyan() : dim);
    lab_show(volt, volt_b, volt_n, v, value);
    lab_show(amp, amp_b, amp_n, a, value);
    lab_show(time, time_b, time_n, t, step->enabled && selected ? lab_cyan() : dim);
}

template <typename Card, typename Area>
void show_field(const PsuSeqEdit* ed, Card& card, Area& value,
                touchgfx::Unicode::UnicodeChar* buf, uint16_t n,
                uint8_t field, const PsuSeqStep* step)
{
    char text[16];
    const int on = ed != 0 && ed->field == field;
    card.setBitmaps(
        touchgfx::Bitmap(on ? BITMAP_CARD_FIELD_SEL_176X72_ID : BITMAP_CARD_FIELD_176X72_ID),
        touchgfx::Bitmap(BITMAP_CARD_FIELD_SEL_176X72_ID));
    card.invalidate();
    if (step == 0)
    {
        lab_show(value, buf, n, "--", lab_muted());
        return;
    }
    if (on)
    {
        psu_seq_edit_cell(text, sizeof(text), ed, step);
        lab_show(value, buf, n, text, ed->fault ? lab_red() : lab_cyan());
    }
    else
    {
        psu_seq_format_field(text, sizeof(text), field, step);
        lab_show(value, buf, n, text, lab_text());
    }
}
}

void ScreenSequencerView::refresh()
{
    char status[56];
    PsuSequencer* seq;
    const PsuSeqStep* edit_step;
    uint8_t count;
    uint8_t sel;
    uint8_t start;
    int row;

    psu_app_ensure();
    seq = psu_sequencer();
    count = seq->count;
    sel = seq->selected;
    if (count == 0U)
        sel = 0U;
    else if (sel >= count)
        sel = (uint8_t)(count - 1U);
    start = 0U;
    if (count > 6U)
    {
        if (sel >= 5U)
            start = (uint8_t)(sel - 5U);
        if ((uint8_t)(start + 6U) > count)
            start = (uint8_t)(count - 6U);
    }
    visible_start = start;
    row = (count == 0U) ? 0 : (int)(sel - start);
    StepHighlight.moveTo(16, 80 + row * 48);
    StepHighlight.setVisible(count > 0U);
    StepHighlight.invalidate();

    show_step(Step1No, Step1NoBuffer, STEP1NO_SIZE, Step1Volt, Step1VoltBuffer, STEP1VOLT_SIZE,
              Step1Amp, Step1AmpBuffer, STEP1AMP_SIZE, Step1Time, Step1TimeBuffer, STEP1TIME_SIZE,
              (uint8_t)(start + 1U), (count > start) ? &seq->steps[start] : 0, row == 0);
    show_step(Step2No, Step2NoBuffer, STEP2NO_SIZE, Step2Volt, Step2VoltBuffer, STEP2VOLT_SIZE,
              Step2Amp, Step2AmpBuffer, STEP2AMP_SIZE, Step2Time, Step2TimeBuffer, STEP2TIME_SIZE,
              (uint8_t)(start + 2U), (count > start + 1U) ? &seq->steps[start + 1U] : 0, row == 1);
    show_step(Step3No, Step3NoBuffer, STEP3NO_SIZE, Step3Volt, Step3VoltBuffer, STEP3VOLT_SIZE,
              Step3Amp, Step3AmpBuffer, STEP3AMP_SIZE, Step3Time, Step3TimeBuffer, STEP3TIME_SIZE,
              (uint8_t)(start + 3U), (count > start + 2U) ? &seq->steps[start + 2U] : 0, row == 2);
    show_step(Step4No, Step4NoBuffer, STEP4NO_SIZE, Step4Volt, Step4VoltBuffer, STEP4VOLT_SIZE,
              Step4Amp, Step4AmpBuffer, STEP4AMP_SIZE, Step4Time, Step4TimeBuffer, STEP4TIME_SIZE,
              (uint8_t)(start + 4U), (count > start + 3U) ? &seq->steps[start + 3U] : 0, row == 3);
    show_step(Step5No, Step5NoBuffer, STEP5NO_SIZE, Step5Volt, Step5VoltBuffer, STEP5VOLT_SIZE,
              Step5Amp, Step5AmpBuffer, STEP5AMP_SIZE, Step5Time, Step5TimeBuffer, STEP5TIME_SIZE,
              (uint8_t)(start + 5U), (count > start + 4U) ? &seq->steps[start + 4U] : 0, row == 4);
    show_step(Step6No, Step6NoBuffer, STEP6NO_SIZE, Step6Volt, Step6VoltBuffer, STEP6VOLT_SIZE,
              Step6Amp, Step6AmpBuffer, STEP6AMP_SIZE, Step6Time, Step6TimeBuffer, STEP6TIME_SIZE,
              (uint8_t)(start + 6U), (count > start + 5U) ? &seq->steps[start + 5U] : 0, row == 5);

    edit_step = (count > 0U) ? &seq->steps[sel] : 0;
    show_field(&edit, EditCardV, EditVolt, EditVoltBuffer, EDITVOLT_SIZE, PSU_SEQ_FIELD_V, edit_step);
    show_field(&edit, EditCardI, EditAmp, EditAmpBuffer, EDITAMP_SIZE, PSU_SEQ_FIELD_I, edit_step);
    show_field(&edit, EditCardT, EditTime, EditTimeBuffer, EDITTIME_SIZE, PSU_SEQ_FIELD_T, edit_step);
    show_field(&edit, EditCardS, EditSlew, EditSlewBuffer, EDITSLEW_SIZE, PSU_SEQ_FIELD_S, edit_step);
    LblEditV.setColor(edit.field == PSU_SEQ_FIELD_V ? lab_cyan() : lab_muted());
    LblEditI.setColor(edit.field == PSU_SEQ_FIELD_I ? lab_cyan() : lab_muted());
    LblEditT.setColor(edit.field == PSU_SEQ_FIELD_T ? lab_cyan() : lab_muted());
    LblEditS.setColor(edit.field == PSU_SEQ_FIELD_S ? lab_cyan() : lab_muted());
    LblEditV.invalidate();
    LblEditI.invalidate();
    LblEditT.invalidate();
    LblEditS.invalidate();
    {
        const int locked = psu_seq_edit_locked(seq);
        setKeys(edit.field != PSU_SEQ_FIELD_NONE, locked == 0);
    }

    if (psu_seq_edit_locked(seq))
    {
        lab_show(SeqStatus, SeqStatusBuffer, SEQSTATUS_SIZE, "LOCKED", lab_amber());
        return;
    }
    if (edit.fault)
    {
        lab_show(SeqStatus, SeqStatusBuffer, SEQSTATUS_SIZE, "RANGE", lab_red());
        return;
    }
    if (seq->status[0] != '\0')
        (void)snprintf(status, sizeof(status), "%s", seq->status);
    else if (seq->run == PSU_SEQ_RUN)
        (void)snprintf(status, sizeof(status), "RUN  %u/%u", (unsigned)(seq->step_index + 1U), (unsigned)count);
    else if (seq->run == PSU_SEQ_PAUSE)
        (void)snprintf(status, sizeof(status), "PAUSED");
    else if (seq->run == PSU_SEQ_DONE)
        (void)snprintf(status, sizeof(status), "DONE");
    else if (seq->run == PSU_SEQ_ABORTED)
        (void)snprintf(status, sizeof(status), "ABORTED");
    else
        (void)snprintf(status, sizeof(status), "IDLE");
    lab_show(SeqStatus, SeqStatusBuffer, SEQSTATUS_SIZE, status,
             (seq->run == PSU_SEQ_ABORTED) ? lab_red() : lab_cyan());
}

void ScreenSequencerView::setKeys(bool on, bool enabled)
{
    const bool touch = on && enabled;
    SeqKey1.setTouchable(touch);
    SeqKey2.setTouchable(touch);
    SeqKey3.setTouchable(touch);
    SeqKey4.setTouchable(touch);
    SeqKey5.setTouchable(touch);
    SeqKey6.setTouchable(touch);
    SeqKey7.setTouchable(touch);
    SeqKey8.setTouchable(touch);
    SeqKey9.setTouchable(touch);
    SeqKey0.setTouchable(touch);
    SeqKeyClr.setTouchable(touch);
    SeqKeyDel.setTouchable(touch);
    SeqKeyDot.setTouchable(touch);
    SeqKeyApply.setTouchable(touch);
}

void ScreenSequencerView::poseKeypad(int16_t y, int16_t alpha)
{
    if (alpha < 0)
        alpha = 0;
    if (alpha > 255)
        alpha = 255;
    keypadAlpha = alpha;
    SeqKeypad.moveTo(KEYPAD_X, y);
    const uint8_t a = (uint8_t)alpha;
    SeqKey1.setAlpha(a);
    SeqKey2.setAlpha(a);
    SeqKey3.setAlpha(a);
    SeqKey4.setAlpha(a);
    SeqKey5.setAlpha(a);
    SeqKey6.setAlpha(a);
    SeqKey7.setAlpha(a);
    SeqKey8.setAlpha(a);
    SeqKey9.setAlpha(a);
    SeqKey0.setAlpha(a);
    SeqKeyClr.setAlpha(a);
    SeqKeyDel.setAlpha(a);
    SeqKeyDot.setAlpha(a);
    SeqKeyApply.setAlpha(a);
}

void ScreenSequencerView::seqStepKeypad()
{
    int16_t y;
    int16_t alpha;
    if (!keypadMoving)
        return;
    if (keypadTick < KEYPAD_TICKS)
        keypadTick++;
    {
        const int16_t dy = (int16_t)(keypadToY - keypadFromY);
        const int16_t da = (int16_t)(keypadToA - keypadFromA);
        const bool inward = keypadToY <= keypadFromY;
        y = inward ? easeOut(keypadTick, keypadFromY, dy, KEYPAD_TICKS)
                   : easeIn(keypadTick, keypadFromY, dy, KEYPAD_TICKS);
        alpha = inward ? easeOut(keypadTick, keypadFromA, da, KEYPAD_TICKS)
                       : easeIn(keypadTick, keypadFromA, da, KEYPAD_TICKS);
    }
    poseKeypad(y, alpha);
    if (keypadTick < KEYPAD_TICKS)
        return;
    keypadMoving = false;
    if (keypadToA == 0)
    {
        SeqKeypad.setVisible(false);
        SeqKeypad.invalidate();
    }
}

void ScreenSequencerView::seqSyncKeypad()
{
    const bool show = edit.field != PSU_SEQ_FIELD_NONE;
    const int16_t targetY = show ? KEYPAD_SHOWN_Y : KEYPAD_HIDDEN_Y;
    const int16_t targetA = show ? 255 : 0;
    if (keypadMoving && keypadToY == targetY && keypadToA == targetA)
        return;
    if (!keypadMoving && SeqKeypad.getY() == targetY && keypadAlpha == targetA)
    {
        SeqKeypad.setVisible(show);
        if (!show)
            SeqKeypad.invalidate();
        return;
    }
    keypadFromY = SeqKeypad.getY();
    keypadFromA = keypadAlpha;
    keypadToY = targetY;
    keypadToA = targetA;
    keypadTick = 0;
    keypadMoving = true;
    if (show)
    {
        SeqKeypad.setVisible(true);
        SeqKeypad.invalidate();
    }
}

void ScreenSequencerView::syncEdit()
{
    if (edit.field != PSU_SEQ_FIELD_NONE)
        psu_seq_edit_select(&edit, psu_sequencer(), edit.field);
}

void ScreenSequencerView::chooseField(uint8_t field)
{
    if (edit.field == field)
    {
        edit.field = PSU_SEQ_FIELD_NONE;
        edit.fault = 0;
    }
    else
        psu_seq_edit_select(&edit, psu_sequencer(), field);
    refresh();
    seqSyncKeypad();
}

void ScreenSequencerView::pickRow(uint8_t row)
{
    PsuSequencer* seq = psu_sequencer();
    uint8_t index = (uint8_t)(visible_start + row);
    if (seq->count == 0U || index >= seq->count)
        return;
    seq->selected = index;
    syncEdit();
    refresh();
}

void ScreenSequencerView::seqRun()
{
    psu_seq_start(psu_sequencer(), psu_app_now());
    refresh();
}
void ScreenSequencerView::seqPause()
{
    psu_seq_pause(psu_sequencer(), psu_app_now());
    refresh();
}
void ScreenSequencerView::seqStop()
{
    psu_seq_stop(psu_sequencer(), psu_app_now(), 0);
    refresh();
}
void ScreenSequencerView::seqAdd()
{
    psu_seq_add(psu_sequencer());
    syncEdit();
    refresh();
}
void ScreenSequencerView::seqRemove()
{
    psu_seq_remove_selected(psu_sequencer());
    syncEdit();
    refresh();
}
void ScreenSequencerView::seqPrev()
{
    psu_seq_edit_prev(&edit, psu_sequencer());
    refresh();
    seqSyncKeypad();
}
void ScreenSequencerView::seqNext()
{
    psu_seq_edit_next(&edit, psu_sequencer());
    refresh();
    seqSyncKeypad();
}
void ScreenSequencerView::seqFieldVolt() { chooseField(PSU_SEQ_FIELD_V); }
void ScreenSequencerView::seqFieldAmp() { chooseField(PSU_SEQ_FIELD_I); }
void ScreenSequencerView::seqFieldTime() { chooseField(PSU_SEQ_FIELD_T); }
void ScreenSequencerView::seqFieldSlew() { chooseField(PSU_SEQ_FIELD_S); }
void ScreenSequencerView::seqPick1() { pickRow(0); }
void ScreenSequencerView::seqPick2() { pickRow(1); }
void ScreenSequencerView::seqPick3() { pickRow(2); }
void ScreenSequencerView::seqPick4() { pickRow(3); }
void ScreenSequencerView::seqPick5() { pickRow(4); }
void ScreenSequencerView::seqPick6() { pickRow(5); }
void ScreenSequencerView::seqKey0() { psu_seq_edit_key(&edit, psu_sequencer(), '0'); refresh(); }
void ScreenSequencerView::seqKey1() { psu_seq_edit_key(&edit, psu_sequencer(), '1'); refresh(); }
void ScreenSequencerView::seqKey2() { psu_seq_edit_key(&edit, psu_sequencer(), '2'); refresh(); }
void ScreenSequencerView::seqKey3() { psu_seq_edit_key(&edit, psu_sequencer(), '3'); refresh(); }
void ScreenSequencerView::seqKey4() { psu_seq_edit_key(&edit, psu_sequencer(), '4'); refresh(); }
void ScreenSequencerView::seqKey5() { psu_seq_edit_key(&edit, psu_sequencer(), '5'); refresh(); }
void ScreenSequencerView::seqKey6() { psu_seq_edit_key(&edit, psu_sequencer(), '6'); refresh(); }
void ScreenSequencerView::seqKey7() { psu_seq_edit_key(&edit, psu_sequencer(), '7'); refresh(); }
void ScreenSequencerView::seqKey8() { psu_seq_edit_key(&edit, psu_sequencer(), '8'); refresh(); }
void ScreenSequencerView::seqKey9() { psu_seq_edit_key(&edit, psu_sequencer(), '9'); refresh(); }
void ScreenSequencerView::seqKeyDot() { psu_seq_edit_key(&edit, psu_sequencer(), '.'); refresh(); }
void ScreenSequencerView::seqKeyClr() { psu_seq_edit_clear(&edit, psu_sequencer()); refresh(); }
void ScreenSequencerView::seqKeyDel() { psu_seq_edit_backspace(&edit, psu_sequencer()); refresh(); }
void ScreenSequencerView::seqKeyApply() { psu_seq_edit_apply(&edit, psu_sequencer()); refresh(); }
