#include "psu_energy.h"
#include <stdio.h>
#include <string.h>

void psu_energy_reset(PsuEnergySession *s)
{
  if (s == 0)
    return;
  memset(s, 0, sizeof(*s));
}

static void add_mwh(uint32_t *mwh, uint64_t *resid, uint64_t mw_ms)
{
  uint64_t sum = *resid + mw_ms;
  *mwh += (uint32_t)(sum / 3600000ULL);
  *resid = sum % 3600000ULL;
}

static void close_session(PsuEnergySession *s)
{
  if (!s->active)
  {
    s->connected = 0U;
    s->last_dir = 0U;
    s->in_gap = 0U;
    return;
  }
  s->closed_live_ms = s->live_ms;
  s->closed_in_mwh = s->in_mwh;
  s->closed_out_mwh = s->out_mwh;
  s->closed_gaps = s->gap_count;
  s->has_closed = 1U;
  s->active = 0U;
  s->connected = 0U;
  s->have_sample = 0U;
  s->in_gap = 0U;
  s->last_dir = 0U;
  s->live_ms = 0U;
  s->gap_ms = 0U;
  s->gap_count = 0U;
  s->last_ms = 0U;
  s->in_mwh = 0U;
  s->out_mwh = 0U;
  s->in_resid = 0U;
  s->out_resid = 0U;
}

void psu_energy_observe(PsuEnergySession *s, uint32_t now_ms, int connected,
                        int sample_valid, int32_t power_mw)
{
  uint32_t dt;
  int64_t mag;
  if (s == 0)
    return;
  if (!connected)
  {
    close_session(s);
    return;
  }
  if (!s->active)
  {
    s->active = 1U;
    s->connected = 1U;
    s->have_sample = 0U;
    s->in_gap = 0U;
    s->last_dir = 0U;
    s->live_ms = 0U;
    s->gap_ms = 0U;
    s->gap_count = 0U;
    s->last_ms = 0U;
    s->in_mwh = 0U;
    s->out_mwh = 0U;
    s->in_resid = 0U;
    s->out_resid = 0U;
  }
  s->connected = 1U;
  if (!sample_valid)
  {
    if (s->have_sample && !s->in_gap)
      s->in_gap = 1U;
    s->last_dir = 0U;
    return;
  }
  if (power_mw > 0)
    s->last_dir = 2U;
  else if (power_mw < 0)
    s->last_dir = 1U;
  else
    s->last_dir = 0U;
  if (!s->have_sample || now_ms < s->last_ms)
  {
    s->have_sample = 1U;
    s->in_gap = 0U;
    s->last_ms = now_ms;
    return;
  }
  dt = now_ms - s->last_ms;
  s->last_ms = now_ms;
  if (s->in_gap || dt > PSU_ENERGY_GAP_MS)
  {
    s->gap_count++;
    s->gap_ms += dt;
    s->in_gap = 0U;
    return;
  }
  s->live_ms += dt;
  mag = power_mw < 0 ? -(int64_t)power_mw : (int64_t)power_mw;
  if (power_mw > 0)
    add_mwh(&s->out_mwh, &s->out_resid, (uint64_t)mag * (uint64_t)dt);
  else if (power_mw < 0)
    add_mwh(&s->in_mwh, &s->in_resid, (uint64_t)mag * (uint64_t)dt);
}

static void wh_text(char *dst, size_t n, uint32_t mwh)
{
  (void)snprintf(dst, n, "%u.%02u Wh", (unsigned)(mwh / 1000U),
                 (unsigned)((mwh % 1000U) / 10U));
}

void psu_energy_format(const PsuEnergySession *s, char *dst, size_t n)
{
  char in_wh[16];
  char out_wh[16];
  const PsuEnergySession *show = s;
  PsuEnergySession closed;
  const char *kind;
  if (dst == 0 || n == 0U)
    return;
  if (s == 0 || (!s->active && !s->has_closed && !s->have_sample))
  {
    (void)snprintf(dst, n, "NO PORT SAMPLES");
    return;
  }
  if (!s->active && s->has_closed)
  {
    memset(&closed, 0, sizeof(closed));
    closed.live_ms = s->closed_live_ms;
    closed.in_mwh = s->closed_in_mwh;
    closed.out_mwh = s->closed_out_mwh;
    closed.gap_count = s->closed_gaps;
    show = &closed;
    kind = "LAST";
  }
  else if (!s->have_sample)
  {
    (void)snprintf(dst, n, "NO PORT SAMPLES");
    return;
  }
  else
  {
    kind = "LIVE";
  }
  wh_text(in_wh, sizeof(in_wh), show->in_mwh);
  wh_text(out_wh, sizeof(out_wh), show->out_mwh);
  if (show->gap_count)
  {
    (void)snprintf(dst, n, "%s %u.%u s  IN %s  OUT %s  GAP %u",
                   kind, (unsigned)(show->live_ms / 1000U),
                   (unsigned)((show->live_ms % 1000U) / 100U),
                   in_wh, out_wh, (unsigned)show->gap_count);
  }
  else
  {
    (void)snprintf(dst, n, "%s %u.%u s  IN %s  OUT %s",
                   kind, (unsigned)(show->live_ms / 1000U),
                   (unsigned)((show->live_ms % 1000U) / 100U),
                   in_wh, out_wh);
  }
}
