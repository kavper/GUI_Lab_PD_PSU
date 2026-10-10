#include "psu_seq.h"
#include "psu_limits.h"
#include "psu_format.h"
#include <stdio.h>
#include <string.h>

static void set_status(PsuSequencer *seq, const char *text)
{
  (void)snprintf(seq->status, sizeof(seq->status), "%s", text);
}

static int step_valid(const PsuSeqStep *step)
{
  if (step->voltage_mv > PSU_VOLTAGE_MAX_MV || step->current_ma > PSU_CURRENT_MAX_MA)
    return 0;
  if (step->time_ms < 100U || step->time_ms > 3600000U)
    return 0;
  if (step->slew_mv_per_s > 100000U)
    return 0;
  if (step->output_action > PSU_STEP_OFF)
    return 0;
  return 1;
}

void psu_seq_init(PsuSequencer *seq)
{
  if (seq == 0)
    return;
  memset(seq, 0, sizeof(*seq));
  seq->count = 1U;
  seq->steps[0].voltage_mv = 5000U;
  seq->steps[0].current_ma = 1000U;
  seq->steps[0].time_ms = 1000U;
  seq->steps[0].enabled = 1U;
  seq->steps[0].output_action = PSU_STEP_KEEP;
  seq->stop_action = PSU_STOP_OFF;
  seq->mode = PSU_SEQ_ONCE;
  seq->loops_requested = 1U;
  set_status(seq, "IDLE");
}

int psu_seq_add(PsuSequencer *seq)
{
  PsuSeqStep copy;
  if (seq == 0 || seq->config_locked || seq->count >= PSU_SEQ_MAX_STEPS)
    return 0;
  copy = seq->steps[seq->selected];
  seq->steps[seq->count] = copy;
  seq->selected = seq->count;
  ++seq->count;
  return 1;
}

int psu_seq_remove_selected(PsuSequencer *seq)
{
  uint8_t i;
  if (seq == 0 || seq->config_locked || seq->count <= 1U)
    return 0;
  for (i = seq->selected; i + 1U < seq->count; ++i)
    seq->steps[i] = seq->steps[i + 1U];
  --seq->count;
  if (seq->selected >= seq->count)
    seq->selected = (uint8_t)(seq->count - 1U);
  memset(&seq->steps[seq->count], 0, sizeof(seq->steps[0]));
  return 1;
}

int psu_seq_set_step(PsuSequencer *seq, uint8_t index, const PsuSeqStep *step)
{
  if (seq == 0 || step == 0 || seq->config_locked || index >= seq->count)
    return 0;
  if (!step_valid(step))
    return 0;
  seq->steps[index] = *step;
  return 1;
}

static void apply_stop(PsuSequencer *seq)
{
  if (seq->stop_action == PSU_STOP_OFF)
  {
    if (seq->io.output)
      seq->io.output(0, seq->io.user);
  }
  else if (seq->stop_action == PSU_STOP_START && seq->io.limits)
  {
    seq->io.limits(seq->steps[0].voltage_mv, seq->steps[0].current_ma, seq->io.user);
  }
}

int psu_seq_start(PsuSequencer *seq, uint32_t now_ms)
{
  if (seq == 0 || seq->count == 0U)
    return 0;
  {uint8_t i,enabled=0;for(i=0;i<seq->count;i++){if(!step_valid(&seq->steps[i])){set_status(seq,"INVALID STEP");return 0;}enabled|=seq->steps[i].enabled;}
   if(!enabled){set_status(seq,"NO ENABLED STEPS");return 0;}}
  seq->run = PSU_SEQ_RUN;
  seq->step_index = 0U;
  seq->loop_index = 1U;
  seq->step_started_ms = now_ms;
  seq->seq_started_ms = now_ms;
  seq->pause_accum_ms = 0U;
  seq->waiting_readback = 0U;
  seq->command_valid = seq->output_issued = seq->waiting_output = 0U;
  seq->command_ms = 0U;
  seq->config_locked = 1U;
  seq->ramp_mv = seq->steps[0].voltage_mv;
  set_status(seq, "RUN");
  return 1;
}

void psu_seq_pause(PsuSequencer *seq, uint32_t now_ms)
{
  if (seq == 0 || seq->run != PSU_SEQ_RUN)
    return;
  seq->pause_accum_ms = now_ms;
  seq->run = PSU_SEQ_PAUSE;
  set_status(seq, "PAUSED");
}

void psu_seq_resume(PsuSequencer *seq, uint32_t now_ms)
{
  uint32_t held;
  if (seq == 0 || seq->run != PSU_SEQ_PAUSE)
    return;
  held = now_ms - seq->pause_accum_ms;
  seq->step_started_ms += held;
  if(seq->command_valid)seq->command_ms += held;
  seq->seq_started_ms += held;
  seq->run = PSU_SEQ_RUN;
  set_status(seq, "RUN");
}

void psu_seq_stop(PsuSequencer *seq, uint32_t now_ms, int abort_timeout)
{
  (void)now_ms;
  if (seq == 0)
    return;
  apply_stop(seq);
  seq->run = abort_timeout ? PSU_SEQ_ABORTED : PSU_SEQ_IDLE;
  seq->config_locked = 0U;
  seq->waiting_readback = 0U;
  set_status(seq, abort_timeout ? "ABORTED / CONTROLLER TIMEOUT" : "STOP");
}

static int next_enabled(const PsuSequencer *seq, uint8_t from, uint8_t *out)
{
  uint8_t i;
  for (i = from; i < seq->count; ++i)
  {
    if (seq->steps[i].enabled)
    {
      *out = i;
      return 1;
    }
  }
  return 0;
}

static void emit_step(PsuSequencer *seq, const PsuSeqStep *step, uint32_t voltage_mv, uint32_t now_ms)
{
  /* SET must be acknowledged before ON; G4 obtains physical PERMIT itself. */
  seq->command_valid=1U;
  if (seq->io.limits)
    seq->io.limits(voltage_mv, step->current_ma, seq->io.user);
  seq->waiting_readback = 1U;
  seq->command_ms = now_ms;
}

void psu_seq_tick(PsuSequencer *seq, uint32_t now_ms)
{
  PsuSeqStep *step;
  uint8_t idx;
  uint32_t elapsed;
  if (seq == 0 || seq->run != PSU_SEQ_RUN)
    return;
  if (!next_enabled(seq, seq->step_index, &idx))
  {
    if (seq->mode == PSU_SEQ_INFINITE ||
        (seq->mode == PSU_SEQ_N && seq->loop_index < seq->loops_requested))
    {
      if(seq->loop_index<65535U)++seq->loop_index;
      seq->step_index = 0U;
      seq->step_started_ms = now_ms;
      seq->waiting_readback = 0U;
      seq->command_valid=seq->output_issued=seq->waiting_output=0U;
      return;
    }
    apply_stop(seq);
    seq->run = PSU_SEQ_DONE;
    seq->config_locked = 0U;
    set_status(seq, "DONE");
    return;
  }
  if (idx != seq->step_index)
  {
    seq->step_index = idx;
    seq->step_started_ms = now_ms;
    seq->waiting_readback = 0U;
    seq->ramp_mv = 0U;
  }
  step = &seq->steps[seq->step_index];
  if (!seq->command_valid && !seq->waiting_readback)
  {
    seq->ramp_mv = (step->slew_mv_per_s == 0U) ? step->voltage_mv : 0U;
    emit_step(seq, step, seq->ramp_mv, now_ms);
    if(seq->run!=PSU_SEQ_RUN)return;
  }
  if (step->slew_mv_per_s != 0U && seq->ramp_mv < step->voltage_mv)
  {
    uint32_t dt = now_ms - seq->command_ms;
    uint32_t add = (step->slew_mv_per_s * dt) / 1000U;
    uint32_t next = seq->ramp_mv + add;
    if (dt >= 120U)
    {
      if (next > step->voltage_mv)
        next = step->voltage_mv;
      seq->ramp_mv = next;
      seq->command_ms = now_ms;
      if (seq->io.limits)
        seq->io.limits(seq->ramp_mv, step->current_ma, seq->io.user);
    }
  }
  if (seq->waiting_readback)
  {
    uint32_t got_mv = 0, got_ma = 0;
    int have = seq->io.applied ? seq->io.applied(&got_mv, &got_ma, seq->io.user) : 0;
    if (have && got_mv == seq->ramp_mv && got_ma == step->current_ma) {
      seq->waiting_readback = 0U;
      seq->step_started_ms=now_ms;
    }
    else if ((now_ms - seq->step_started_ms) > 500U && seq->command_ms != 0U &&
             (now_ms - seq->command_ms) > 500U)
    {
      psu_seq_stop(seq, now_ms, 1);
      return;
    }
  }
  if(!seq->waiting_readback && !seq->output_issued){
    seq->output_issued=1U;
    if(step->output_action!=PSU_STEP_KEEP && seq->io.output){
      const int on=step->output_action==PSU_STEP_ON;
      if(on && seq->io.permit_ok && !seq->io.permit_ok(seq->io.user)){psu_seq_stop(seq,now_ms,1);set_status(seq,"ON REJECTED");return;}
      seq->waiting_output=1U;seq->command_ms=now_ms;
      seq->io.output(on,seq->io.user);
      if(seq->run!=PSU_SEQ_RUN)return;
    }
  }
  if(seq->waiting_output){
    if(!seq->io.output_ready || seq->io.output_ready(step->output_action==PSU_STEP_ON,seq->io.user)){
      seq->waiting_output=0U;seq->step_started_ms=now_ms;
    }else if(now_ms-seq->command_ms>5000U){psu_seq_stop(seq,now_ms,1);set_status(seq,"G4/G0 START TIMEOUT");return;}
    else {set_status(seq,"WAITING FOR G4 / G0");return;}
  }
  elapsed = now_ms - seq->step_started_ms;
  if (!seq->waiting_readback && elapsed >= step->time_ms)
  {
    uint8_t nidx = (uint8_t)(seq->step_index + 1U);
    seq->step_index = nidx;
    seq->step_started_ms = now_ms;
    seq->waiting_readback = 0U;
    seq->command_ms = 0U;
    seq->command_valid=seq->output_issued=seq->waiting_output=0U;
    (void)snprintf(seq->status, sizeof(seq->status), "STEP %u", (unsigned)(nidx + 1U));
  }
}

int psu_seq_edit_locked(const PsuSequencer *seq)
{
  if (seq == 0)
    return 1;
  return seq->config_locked != 0U || seq->run == PSU_SEQ_RUN || seq->run == PSU_SEQ_PAUSE;
}

void psu_seq_edit_init(PsuSeqEdit *ed)
{
  if (ed == 0)
    return;
  memset(ed, 0, sizeof(*ed));
}

static const PsuSeqStep *selected_step(const PsuSequencer *seq)
{
  if (seq == 0 || seq->count == 0U)
    return 0;
  if (seq->selected >= seq->count)
    return &seq->steps[seq->count - 1U];
  return &seq->steps[seq->selected];
}

static uint32_t field_milli(const PsuSeqStep *step, uint8_t field)
{
  if (step == 0)
    return 0U;
  if (field == PSU_SEQ_FIELD_V)
    return step->voltage_mv;
  if (field == PSU_SEQ_FIELD_I)
    return step->current_ma;
  if (field == PSU_SEQ_FIELD_T)
    return step->time_ms;
  if (field == PSU_SEQ_FIELD_S)
    return step->slew_mv_per_s;
  return 0U;
}

static void format_time(char *dst, size_t n, uint32_t ms)
{
  if (ms >= 1000U && (ms % 1000U) == 0U)
    (void)snprintf(dst, n, "%u s", (unsigned)(ms / 1000U));
  else if (ms >= 1000U)
    (void)snprintf(dst, n, "%u.%u s", (unsigned)(ms / 1000U), (unsigned)((ms % 1000U) / 100U));
  else
    (void)snprintf(dst, n, "%u ms", (unsigned)ms);
}

static void format_slew(char *dst, size_t n, uint32_t mv_per_s)
{
  if (mv_per_s == 0U)
    (void)snprintf(dst, n, "0 V/s");
  else
    (void)snprintf(dst, n, "%u.%03u V/s", (unsigned)(mv_per_s / 1000U), (unsigned)(mv_per_s % 1000U));
}

void psu_seq_format_field(char *dst, size_t n, uint8_t field, const PsuSeqStep *step)
{
  if (dst == 0 || n == 0U)
    return;
  dst[0] = '\0';
  if (step == 0)
  {
    (void)snprintf(dst, n, "--");
    return;
  }
  if (field == PSU_SEQ_FIELD_V)
    psu_format_voltage(dst, n, step->voltage_mv);
  else if (field == PSU_SEQ_FIELD_I)
    psu_format_current_ma(dst, n, step->current_ma);
  else if (field == PSU_SEQ_FIELD_T)
    format_time(dst, n, step->time_ms);
  else if (field == PSU_SEQ_FIELD_S)
    format_slew(dst, n, step->slew_mv_per_s);
}

void psu_seq_edit_cell(char *dst, size_t n, const PsuSeqEdit *ed, const PsuSeqStep *step)
{
  if (dst == 0 || n == 0U)
    return;
  if (ed != 0 && ed->fault)
  {
    (void)snprintf(dst, n, "RANGE");
    return;
  }
  if (ed != 0 && ed->field != PSU_SEQ_FIELD_NONE && ed->editor.replace_on_next == 0U)
  {
    (void)snprintf(dst, n, "%s", ed->editor.text);
    return;
  }
  psu_seq_format_field(dst, n, ed ? ed->field : PSU_SEQ_FIELD_NONE, step);
}

int psu_seq_edit_select(PsuSeqEdit *ed, const PsuSequencer *seq, uint8_t field)
{
  const PsuSeqStep *step;
  if (ed == 0 || field < PSU_SEQ_FIELD_V || field > PSU_SEQ_FIELD_S)
    return 0;
  step = selected_step(seq);
  if (step == 0)
    return 0;
  ed->field = field;
  ed->fault = 0U;
  psu_editor_load_milli(&ed->editor, field_milli(step, field), 3);
  return 1;
}

int psu_seq_edit_prev(PsuSeqEdit *ed, const PsuSequencer *seq)
{
  uint8_t field;
  if (ed == 0)
    return 0;
  field = ed->field;
  if (field < PSU_SEQ_FIELD_V || field > PSU_SEQ_FIELD_S)
    field = PSU_SEQ_FIELD_V;
  field = (field == PSU_SEQ_FIELD_V) ? PSU_SEQ_FIELD_S : (uint8_t)(field - 1U);
  return psu_seq_edit_select(ed, seq, field);
}

int psu_seq_edit_next(PsuSeqEdit *ed, const PsuSequencer *seq)
{
  uint8_t field;
  if (ed == 0)
    return 0;
  field = ed->field;
  if (field < PSU_SEQ_FIELD_V || field > PSU_SEQ_FIELD_S)
    field = PSU_SEQ_FIELD_S;
  field = (field == PSU_SEQ_FIELD_S) ? PSU_SEQ_FIELD_V : (uint8_t)(field + 1U);
  return psu_seq_edit_select(ed, seq, field);
}

static int edit_ready(PsuSeqEdit *ed, const PsuSequencer *seq)
{
  if (ed == 0 || ed->field == PSU_SEQ_FIELD_NONE)
    return 0;
  if (psu_seq_edit_locked(seq))
    return 0;
  return 1;
}

int psu_seq_edit_key(PsuSeqEdit *ed, const PsuSequencer *seq, char key)
{
  if (!edit_ready(ed, seq))
    return 0;
  if (ed->fault)
  {
    ed->fault = 0U;
    ed->editor.replace_on_next = 1U;
  }
  psu_editor_key(&ed->editor, key);
  return 1;
}

int psu_seq_edit_clear(PsuSeqEdit *ed, const PsuSequencer *seq)
{
  if (!edit_ready(ed, seq))
    return 0;
  ed->fault = 0U;
  psu_editor_clear(&ed->editor);
  return 1;
}

int psu_seq_edit_backspace(PsuSeqEdit *ed, const PsuSequencer *seq)
{
  if (!edit_ready(ed, seq))
    return 0;
  ed->fault = 0U;
  psu_editor_backspace(&ed->editor);
  return 1;
}

int psu_seq_edit_apply(PsuSeqEdit *ed, PsuSequencer *seq)
{
  uint32_t milli = 0U;
  PsuSeqStep step;
  uint8_t index;
  if (ed == 0 || seq == 0 || ed->field == PSU_SEQ_FIELD_NONE || seq->count == 0U)
    return 0;
  if (psu_seq_edit_locked(seq))
    return 0;
  index = seq->selected < seq->count ? seq->selected : (uint8_t)(seq->count - 1U);
  if (!psu_editor_parse_milli(&ed->editor, &milli)) {
    psu_editor_clear(&ed->editor);ed->fault=0;return 0;
  }
  uint32_t maximum = ed->field==PSU_SEQ_FIELD_V?27000U:ed->field==PSU_SEQ_FIELD_I?5000U:ed->field==PSU_SEQ_FIELD_T?3600000U:100000U;
  if(milli>maximum)milli=maximum;
  if(ed->field==PSU_SEQ_FIELD_T && milli<100U)milli=100U;
  step = seq->steps[index];
  if (ed->field == PSU_SEQ_FIELD_V)
    step.voltage_mv = milli;
  else if (ed->field == PSU_SEQ_FIELD_I)
    step.current_ma = milli;
  else if (ed->field == PSU_SEQ_FIELD_T)
    step.time_ms = milli;
  else
    step.slew_mv_per_s = milli;
  if (!psu_seq_set_step(seq, index, &step))
  {
    ed->fault = 1U;
    return 0;
  }
  ed->fault = 0U;
  psu_editor_load_milli(&ed->editor, milli, 3);
  return 1;
}
