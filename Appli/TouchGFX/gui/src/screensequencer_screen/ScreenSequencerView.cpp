#include <gui/screensequencer_screen/ScreenSequencerView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_format.h"
}

ScreenSequencerView::ScreenSequencerView()
    : divider(0)
{
}

void ScreenSequencerView::setupScreen()
{
    ScreenSequencerViewBase::setupScreen();
    refresh();
}

void ScreenSequencerView::tearDownScreen()
{
    ScreenSequencerViewBase::tearDownScreen();
}

void ScreenSequencerView::handleTickEvent()
{
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
}

void ScreenSequencerView::refresh()
{
    char buf[24];
    char status[56];
    PsuSequencer* seq;
    uint8_t count;
    uint8_t sel;
    uint8_t start;
    int row;
    const PsuSeqStep* edit;

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

    edit = (count > 0U) ? &seq->steps[sel] : 0;
    if (edit == 0)
    {
        lab_show(EditVolt, EditVoltBuffer, EDITVOLT_SIZE, "--", lab_muted());
        lab_show(EditAmp, EditAmpBuffer, EDITAMP_SIZE, "--", lab_muted());
        lab_show(EditTime, EditTimeBuffer, EDITTIME_SIZE, "--", lab_muted());
        lab_show(EditSlew, EditSlewBuffer, EDITSLEW_SIZE, "--", lab_muted());
    }
    else
    {
        psu_format_voltage(buf, sizeof(buf), edit->voltage_mv);
        lab_show(EditVolt, EditVoltBuffer, EDITVOLT_SIZE, buf, lab_cyan());
        psu_format_current_ma(buf, sizeof(buf), edit->current_ma);
        lab_show(EditAmp, EditAmpBuffer, EDITAMP_SIZE, buf, lab_cyan());
        lab_ms(buf, sizeof(buf), edit->time_ms);
        lab_show(EditTime, EditTimeBuffer, EDITTIME_SIZE, buf, lab_text());
        lab_slew(buf, sizeof(buf), edit->slew_mv_per_s);
        lab_show(EditSlew, EditSlewBuffer, EDITSLEW_SIZE, buf, lab_text());
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
    refresh();
}
void ScreenSequencerView::seqRemove()
{
    psu_seq_remove_selected(psu_sequencer());
    refresh();
}
void ScreenSequencerView::seqPrev()
{
    if (psu_sequencer()->selected > 0)
        psu_sequencer()->selected--;
    refresh();
}
void ScreenSequencerView::seqNext()
{
    if (psu_sequencer()->selected + 1 < psu_sequencer()->count)
        psu_sequencer()->selected++;
    refresh();
}
