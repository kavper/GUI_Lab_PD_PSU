#include "psu_seq.h"
#include "psu_limits.h"
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
  seq->run = PSU_SEQ_RUN;
  seq->step_index = 0U;
  seq->loop_index = 1U;
  seq->step_started_ms = now_ms;
  seq->seq_started_ms = now_ms;
  seq->pause_accum_ms = 0U;
  seq->waiting_readback = 0U;
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
  if (step->output_action == PSU_STEP_ON && seq->io.permit_ok && seq->io.permit_ok(seq->io.user) && seq->io.output)
    seq->io.output(1, seq->io.user);
  else if (step->output_action == PSU_STEP_OFF && seq->io.output)
    seq->io.output(0, seq->io.user);
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
      ++seq->loop_index;
      seq->step_index = 0U;
      seq->step_started_ms = now_ms;
      seq->waiting_readback = 0U;
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
  if (seq->command_ms == 0U && !seq->waiting_readback)
  {
    seq->ramp_mv = (step->slew_mv_per_s == 0U) ? step->voltage_mv : 0U;
    emit_step(seq, step, seq->ramp_mv, now_ms);
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
    if (have && got_mv == seq->ramp_mv && got_ma == step->current_ma)
      seq->waiting_readback = 0U;
    else if ((now_ms - seq->step_started_ms) > 500U && seq->command_ms != 0U &&
             (now_ms - seq->command_ms) > 500U)
    {
      psu_seq_stop(seq, now_ms, 1);
      return;
    }
  }
  elapsed = now_ms - seq->step_started_ms;
  if (!seq->waiting_readback && elapsed >= step->time_ms)
  {
    uint8_t nidx = (uint8_t)(seq->step_index + 1U);
    seq->step_index = nidx;
    seq->step_started_ms = now_ms;
    seq->waiting_readback = 0U;
    seq->command_ms = 0U;
    (void)snprintf(seq->status, sizeof(seq->status), "STEP %u", (unsigned)(nidx + 1U));
  }
}
