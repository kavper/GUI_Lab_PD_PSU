#include <gui/common/UiTheme.hpp>
#include <gui/screensequencer_screen/ScreenSequencerView.hpp>
#include <gui/common/LabText.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Bitmap.hpp>
#include <texts/TextKeysAndLanguages.hpp>
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
    psu_seq_edit_select(&edit,psu_sequencer(),PSU_SEQ_FIELD_V);
    refresh();seqSyncKeypad();

    setupTheme();
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
        touchgfx::Bitmap(on ? BITMAP_UX_FIELD_SEL_112X56_ID : BITMAP_UX_FIELD_REL_112X56_ID),
        touchgfx::Bitmap(BITMAP_UX_FIELD_SEL_112X56_ID));
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
    const bool cyclesVisible=edit.field==PSU_SEQ_FIELD_NONE && !cycleEditing;
    CycleCountButton.setVisible(cyclesVisible);CycleLessButton.setVisible(cyclesVisible);CycleMoreButton.setVisible(cyclesVisible);CycleInfinityButton.setVisible(cyclesVisible);CycleValue.setVisible(cyclesVisible);
    CycleCountButton.invalidate();CycleLessButton.invalidate();CycleMoreButton.invalidate();CycleInfinityButton.invalidate();CycleValue.invalidate();
    lab_enable(CycleCountButton,!psu_seq_edit_locked(seq));lab_enable(CycleLessButton,!psu_seq_edit_locked(seq));lab_enable(CycleMoreButton,!psu_seq_edit_locked(seq));lab_enable(CycleInfinityButton,!psu_seq_edit_locked(seq));
    char cycleText[64];
    if(seq->mode==PSU_SEQ_INFINITE)snprintf(cycleText,sizeof(cycleText),"CONTINUOUS / cycle %u",seq->loop_index);
    else snprintf(cycleText,sizeof(cycleText),"%u cycle%s / current %u",seq->mode==PSU_SEQ_ONCE?1:seq->loops_requested,seq->loops_requested==1?"":"s",seq->loop_index);
    lab_show(CycleValue,CycleValueBuffer,CYCLEVALUE_SIZE,cycleText,lab_cyan());
    count = seq->count;
    sel = seq->selected;
    if (count == 0U)
        sel = 0U;
    else if (sel >= count)
        sel = (uint8_t)(count - 1U);
    start = visible_start;
    const uint8_t maxStart = count > 6 ? count - 6 : 0;
    if (start > maxStart) start = maxStart;
    visible_start = start;
    row = (int)sel - start;
    StepHighlight.moveTo(16, 108 + (row >= 0 && row < 6 ? row : 0) * 40);
    StepHighlight.setVisible(count > 0 && row >= 0 && row < 6);
    StepHighlight.invalidate();
    lab_enable(PrevButton,start>0);lab_enable(NextButton,start<maxStart);
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
        setKeys(edit.field != PSU_SEQ_FIELD_NONE || cycleEditing, locked == 0);
        if(cycleEditing){SeqKeyDot.setTouchable(false);lab_show(SeqStatus,SeqStatusBuffer,SEQSTATUS_SIZE,cycleEditor.text,lab_cyan());}
    }

    const bool locked = psu_seq_edit_locked(seq) != 0;
    lab_enable(RunButton, seq->run != PSU_SEQ_RUN && (edit.field == PSU_SEQ_FIELD_NONE || edit.editor.replace_on_next));
    lab_enable(PauseButton, seq->run == PSU_SEQ_RUN);
    lab_enable(StopButton, locked);
    lab_enable(AddButton, !locked && count < PSU_SEQ_MAX_STEPS);
    lab_enable(RemoveButton, !locked && count > 1);
    RunButton.setLabelText(touchgfx::TypedText(seq->run == PSU_SEQ_PAUSE ? T_TXT_SEQ_RESUME : T_TXT_ACT_RUN));
    RunButton.invalidate();
    if (locked) {
        snprintf(status, sizeof(status), "%s  %u / %u", seq->run == PSU_SEQ_PAUSE ? "PAUSED" : "RUNNING", seq->step_index + 1, count);
        lab_show(SeqStatus, SeqStatusBuffer, SEQSTATUS_SIZE, status, lab_amber());
        lab_show(PageFeedback, PageFeedbackBuffer, PAGEFEEDBACK_SIZE, "Sequence active. Stop it before changing steps.", lab_amber());
        return;
    }
    if(cycleEditing)return;
    const char* hint = "Sequence editor";
    if (edit.field == PSU_SEQ_FIELD_V) hint = "Voltage / 0-27 V";
    if (edit.field == PSU_SEQ_FIELD_I) hint = "Current / 0-5 A";
    if (edit.field == PSU_SEQ_FIELD_T) hint = "Hold after ramp / 0.1-3600 s";
    if (edit.field == PSU_SEQ_FIELD_S) hint = "Slew / 0-100 V/s / 0 = immediate";
    char range[100];snprintf(range,sizeof(range),"Steps %u-%u / %u   |   %s",start+1,(start+6<count?start+6:count),count,hint);
    lab_show(PageFeedback, PageFeedbackBuffer, PAGEFEEDBACK_SIZE, range, edit.fault ? lab_amber() : lab_muted());
    if (edit.fault)
    {
        lab_show(SeqStatus, SeqStatusBuffer, SEQSTATUS_SIZE, "CHECK FIELD RANGE", lab_amber());
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
    const bool show = edit.field != PSU_SEQ_FIELD_NONE || cycleEditing;
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
    cycleEditing=false;
    psu_seq_edit_select(&edit, psu_sequencer(), field);
    refresh();
    seqSyncKeypad();
}

void ScreenSequencerView::pickRow(uint8_t row)
{
    if(listDragging)return;
    PsuSequencer* seq = psu_sequencer();
    uint8_t index = (uint8_t)(visible_start + row);
    if (seq->count == 0U || index >= seq->count)
        return;
    seq->selected = index;
    syncEdit();
    refresh();
}

void ScreenSequencerView::seqCycles(){
 if(psu_seq_edit_locked(psu_sequencer()))return;
 cycleEditing=true;edit.field=PSU_SEQ_FIELD_NONE;
 psu_editor_load_milli(&cycleEditor,psu_sequencer()->loops_requested*1000U,0);
 refresh();seqSyncKeypad();
}
void ScreenSequencerView::seqCycleLess(){PsuSequencer* s=psu_sequencer();if(psu_seq_edit_locked(s))return;if(s->loops_requested>1)--s->loops_requested;s->mode=s->loops_requested==1?PSU_SEQ_ONCE:PSU_SEQ_N;refresh();}
void ScreenSequencerView::seqCycleMore(){PsuSequencer* s=psu_sequencer();if(psu_seq_edit_locked(s))return;if(s->loops_requested<65535)++s->loops_requested;s->mode=s->loops_requested==1?PSU_SEQ_ONCE:PSU_SEQ_N;refresh();}
void ScreenSequencerView::seqInfinity(){if(psu_seq_edit_locked(psu_sequencer()))return;psu_sequencer()->mode=PSU_SEQ_INFINITE;refresh();}
void ScreenSequencerView::cycleKey(char key){if(!psu_seq_edit_locked(psu_sequencer())&&key!='.')psu_editor_key(&cycleEditor,key);}
void ScreenSequencerView::seqRun()
{
    PsuSequencer* seq = psu_sequencer();
    if (edit.field != PSU_SEQ_FIELD_NONE && !edit.editor.replace_on_next) return;
    if (seq->run == PSU_SEQ_PAUSE) psu_seq_resume(seq, psu_app_now());
    else if (seq->run != PSU_SEQ_RUN) {
      if(cycleEditing || psu_charger()->running)return;
      /* RUN explicitly starts the output; KEEP is retained for later steps. */
      for(unsigned i=0;i<seq->count;i++)if(seq->steps[i].enabled){if(seq->steps[i].output_action==PSU_STEP_KEEP)seq->steps[i].output_action=PSU_STEP_ON;break;}
      psu_seq_start(seq, psu_app_now());
    }
    edit.field = PSU_SEQ_FIELD_NONE;
    seqSyncKeypad();
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
    if(psu_sequencer()->selected>=6)visible_start=psu_sequencer()->selected-5;
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
    if(visible_start)--visible_start;
    refresh();
    seqSyncKeypad();
}
void ScreenSequencerView::seqNext()
{
    if(visible_start+6<psu_sequencer()->count)++visible_start;
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
void ScreenSequencerView::seqKey0() { if(cycleEditing)cycleKey('0');else psu_seq_edit_key(&edit, psu_sequencer(), '0'); refresh(); }
void ScreenSequencerView::seqKey1() { if(cycleEditing)cycleKey('1');else psu_seq_edit_key(&edit, psu_sequencer(), '1'); refresh(); }
void ScreenSequencerView::seqKey2() { if(cycleEditing)cycleKey('2');else psu_seq_edit_key(&edit, psu_sequencer(), '2'); refresh(); }
void ScreenSequencerView::seqKey3() { if(cycleEditing)cycleKey('3');else psu_seq_edit_key(&edit, psu_sequencer(), '3'); refresh(); }
void ScreenSequencerView::seqKey4() { if(cycleEditing)cycleKey('4');else psu_seq_edit_key(&edit, psu_sequencer(), '4'); refresh(); }
void ScreenSequencerView::seqKey5() { if(cycleEditing)cycleKey('5');else psu_seq_edit_key(&edit, psu_sequencer(), '5'); refresh(); }
void ScreenSequencerView::seqKey6() { if(cycleEditing)cycleKey('6');else psu_seq_edit_key(&edit, psu_sequencer(), '6'); refresh(); }
void ScreenSequencerView::seqKey7() { if(cycleEditing)cycleKey('7');else psu_seq_edit_key(&edit, psu_sequencer(), '7'); refresh(); }
void ScreenSequencerView::seqKey8() { if(cycleEditing)cycleKey('8');else psu_seq_edit_key(&edit, psu_sequencer(), '8'); refresh(); }
void ScreenSequencerView::seqKey9() { if(cycleEditing)cycleKey('9');else psu_seq_edit_key(&edit, psu_sequencer(), '9'); refresh(); }
void ScreenSequencerView::seqKeyDot() { psu_seq_edit_key(&edit, psu_sequencer(), '.'); refresh(); }
void ScreenSequencerView::seqKeyClr() { if(cycleEditing)psu_editor_clear(&cycleEditor);else psu_seq_edit_clear(&edit, psu_sequencer()); refresh(); }
void ScreenSequencerView::seqKeyDel() { if(cycleEditing)psu_editor_backspace(&cycleEditor);else psu_seq_edit_backspace(&edit, psu_sequencer()); refresh(); }
void ScreenSequencerView::seqKeyApply()
{
    if(cycleEditing){
      uint32_t value;if(!psu_seq_edit_locked(psu_sequencer())&&psu_editor_parse_milli(&cycleEditor,&value)){
       value/=1000;if(value<1)value=1;if(value>65535)value=65535;
       psu_sequencer()->loops_requested=value;psu_sequencer()->mode=value==1?PSU_SEQ_ONCE:PSU_SEQ_N;
       cycleEditing=false;seqSyncKeypad();
      }
    }else psu_seq_edit_apply(&edit, psu_sequencer());
    refresh();
}


extern "C" {
#include "psu_app.h"
}
void ScreenSequencerView::allOff() { psu_app_power_shutdown(); }

void ScreenSequencerView::handleDragEvent(const touchgfx::DragEvent& e) {
 if(e.getOldX()<500 && e.getOldY()>=100 && e.getOldY()<352) {
  listDragging=true;dragPixels+=e.getDeltaY();
  while(dragPixels<=-28){seqNext();dragPixels+=28;}
  while(dragPixels>=28){seqPrev();dragPixels-=28;}
 } else ScreenSequencerViewBase::handleDragEvent(e);
}

void ScreenSequencerView::handleClickEvent(const touchgfx::ClickEvent& e){
 if(e.getType()==touchgfx::ClickEvent::PRESSED){listDragging=false;dragPixels=0;}
 if(e.getType()==touchgfx::ClickEvent::RELEASED&&listDragging){ScreenSequencerViewBase::handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::CANCEL,e.getX(),e.getY()));return;}
 ScreenSequencerViewBase::handleClickEvent(e);
}
void ScreenSequencerView::handleGestureEvent(const touchgfx::GestureEvent& e){
 if(e.getType()==touchgfx::GestureEvent::SWIPE_VERTICAL&&e.getX()<500&&e.getY()>100&&e.getY()<352){if(e.getVelocity()<0)seqNext();else seqPrev();}
 else ScreenSequencerViewBase::handleGestureEvent(e);
}

void ScreenSequencerView::setupTheme()
{
    ui::ThemeScreen& theme=ui::ThemeScreen::get();
    theme.begin(*this);
    theme.box(LabBackground,ui::BACKGROUND);
    theme.box(LabHeader,ui::SURFACE);
    theme.box(ThemeHeaderDivider,ui::BORDER);
    theme.text(ScreenTitle);
    theme.button(BackButton,ui::NORMAL);
    theme.button(StepRow1,ui::NORMAL);
    theme.button(StepRow2,ui::NORMAL);
    theme.button(StepRow3,ui::NORMAL);
    theme.button(StepRow4,ui::NORMAL);
    theme.button(StepRow5,ui::NORMAL);
    theme.button(StepRow6,ui::NORMAL);
    theme.image(StepHighlight,ui::SELECTION);
    theme.text(Step1No);
    theme.text(Step1Volt);
    theme.text(Step1Amp);
    theme.text(Step1Time);
    theme.text(Step2No);
    theme.text(Step2Volt);
    theme.text(Step2Amp);
    theme.text(Step2Time);
    theme.text(Step3No);
    theme.text(Step3Volt);
    theme.text(Step3Amp);
    theme.text(Step3Time);
    theme.text(Step4No);
    theme.text(Step4Volt);
    theme.text(Step4Amp);
    theme.text(Step4Time);
    theme.text(Step5No);
    theme.text(Step5Volt);
    theme.text(Step5Amp);
    theme.text(Step5Time);
    theme.text(Step6No);
    theme.text(Step6Volt);
    theme.text(Step6Amp);
    theme.text(Step6Time);
    theme.button(EditCardV,ui::NORMAL);
    theme.text(LblEditV);
    theme.text(EditVolt);
    theme.button(EditCardI,ui::NORMAL);
    theme.text(LblEditI);
    theme.text(EditAmp);
    theme.button(EditCardT,ui::NORMAL);
    theme.text(LblEditT);
    theme.text(EditTime);
    theme.button(EditCardS,ui::NORMAL);
    theme.text(LblEditS);
    theme.text(EditSlew);
    theme.text(SeqStatus);
    theme.button(RunButton,ui::NORMAL);
    theme.button(PauseButton,ui::NORMAL);
    theme.button(StopButton,ui::DANGER);
    theme.button(AddButton,ui::NORMAL);
    theme.button(RemoveButton,ui::NORMAL);
    theme.button(PrevButton,ui::NORMAL);
    theme.button(NextButton,ui::NORMAL);
    theme.text(PageFeedback);
    theme.text(SeqColumnNo);
    theme.text(SeqColumnV);
    theme.text(SeqColumnI);
    theme.text(SeqColumnT);
    theme.button(AllOffButton,ui::DANGER);
    theme.button(SeqKey0,ui::NORMAL);
    theme.button(SeqKey1,ui::NORMAL);
    theme.button(SeqKey2,ui::NORMAL);
    theme.button(SeqKey3,ui::NORMAL);
    theme.button(SeqKey4,ui::NORMAL);
    theme.button(SeqKey5,ui::NORMAL);
    theme.button(SeqKey6,ui::NORMAL);
    theme.button(SeqKey7,ui::NORMAL);
    theme.button(SeqKey8,ui::NORMAL);
    theme.button(SeqKey9,ui::NORMAL);
    theme.button(SeqKeyDot,ui::NORMAL);
    theme.button(SeqKeyClr,ui::NORMAL);
    theme.button(SeqKeyDel,ui::NORMAL);
    theme.button(SeqKeyApply,ui::PRIMARY);

    theme.button(CycleCountButton,ui::NORMAL);theme.button(CycleLessButton,ui::NORMAL);theme.button(CycleMoreButton,ui::NORMAL);theme.button(CycleInfinityButton,ui::NORMAL);theme.text(CycleValue);
    theme.apply();
}
