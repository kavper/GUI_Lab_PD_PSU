#include "psu_bms.h"
#include "psu_format.h"
#include <stdio.h>
#include <string.h>

static void copy_text(char *dst, size_t n, const char *src)
{
  if (dst == 0 || n == 0U)
    return;
  (void)snprintf(dst, n, "%s", src ? src : "");
}

static void channel_text(char *dst, size_t n, const PsuBmsFrame *frame, unsigned index)
{
  if (!frame->valid || !frame->present[index])
  {
    copy_text(dst, n, "MISSING");
    return;
  }
  if (frame->stale[index])
  {
    copy_text(dst, n, "STALE");
    return;
  }
  psu_format_voltage(dst, n, frame->cell_mv[index]);
}

void psu_bms_face(const PsuBmsFrame *frame, PsuBmsFace *out)
{
  PsuBmsFrame empty;
  unsigned i;
  unsigned live = 0U;
  uint32_t pack = 0U;
  uint16_t lo = 0U;
  uint16_t hi = 0U;
  int spread_ok = 0;
  if (out == 0)
    return;
  memset(out, 0, sizeof(*out));
  if (frame == 0)
  {
    memset(&empty, 0, sizeof(empty));
    frame = &empty;
  }
  for (i = 0; i < 4U; ++i)
    channel_text(out->channel[i], sizeof(out->channel[i]), frame, i);
  (void)snprintf(out->channels_line, sizeof(out->channels_line),
                 "CH1 %s  CH2 %s\nCH3 %s  CH4 %s",
                 out->channel[0], out->channel[1], out->channel[2], out->channel[3]);
  if (!frame->valid)
  {
    copy_text(out->state, sizeof(out->state), "OFFLINE");
    copy_text(out->soc, sizeof(out->soc), "UNAVAILABLE");
    copy_text(out->pack_v, sizeof(out->pack_v), "UNAVAILABLE");
    copy_text(out->pack_i, sizeof(out->pack_i), "UNAVAILABLE");
    copy_text(out->pack_p, sizeof(out->pack_p), "UNAVAILABLE");
    copy_text(out->spread, sizeof(out->spread), "--");
    copy_text(out->temp, sizeof(out->temp), "UNAVAILABLE");
    copy_text(out->balance, sizeof(out->balance), "--");
    copy_text(out->fet_chg, sizeof(out->fet_chg), "--");
    copy_text(out->fet_dsg, sizeof(out->fet_dsg), "--");
    copy_text(out->fault, sizeof(out->fault),
              frame->fault[0] ? frame->fault : "NO CELL FRAME");
  }
  else
  {
    if (frame->fet_chg && frame->fet_dsg)
      copy_text(out->state, sizeof(out->state), "CHG+DSG");
    else if (frame->fet_chg)
      copy_text(out->state, sizeof(out->state), "CHARGE");
    else if (frame->fet_dsg)
      copy_text(out->state, sizeof(out->state), "DISCHARGE");
    else
      copy_text(out->state, sizeof(out->state), "IDLE");
    if (frame->soc_valid)
      (void)snprintf(out->soc, sizeof(out->soc), "%u %%", frame->soc_pct);
    else
      copy_text(out->soc, sizeof(out->soc), "UNAVAILABLE");
    for (i = 0; i < 4U; ++i)
    {
      if (!frame->present[i] || frame->stale[i])
        continue;
      if (!spread_ok)
      {
        lo = frame->cell_mv[i];
        hi = frame->cell_mv[i];
        spread_ok = 1;
      }
      else
      {
        if (frame->cell_mv[i] < lo)
          lo = frame->cell_mv[i];
        if (frame->cell_mv[i] > hi)
          hi = frame->cell_mv[i];
      }
      pack += frame->cell_mv[i];
      live++;
    }
    if (live == 0U)
      copy_text(out->pack_v, sizeof(out->pack_v), "UNAVAILABLE");
    else
      psu_format_voltage(out->pack_v, sizeof(out->pack_v), pack);
    if (live < 2U)
      copy_text(out->spread, sizeof(out->spread), "--");
    else
      (void)snprintf(out->spread, sizeof(out->spread), "%u mV", (unsigned)(hi - lo));
    if (!frame->current_valid)
    {
      copy_text(out->pack_i, sizeof(out->pack_i), "UNAVAILABLE");
      copy_text(out->pack_p, sizeof(out->pack_p), "UNAVAILABLE");
    }
    else
    {
      int32_t ma = frame->current_ma;
      uint32_t abs_ma = ma < 0 ? (uint32_t)(-ma) : (uint32_t)ma;
      char amp[16];
      psu_format_current_ma(amp, sizeof(amp), abs_ma);
      if (ma < 0)
        (void)snprintf(out->pack_i, sizeof(out->pack_i), "-%s", amp);
      else
        (void)snprintf(out->pack_i, sizeof(out->pack_i), "%s", amp);
      if (live == 0U)
        copy_text(out->pack_p, sizeof(out->pack_p), "UNAVAILABLE");
      else
      {
        uint32_t mw = (uint32_t)(((uint64_t)pack * abs_ma) / 1000U);
        psu_format_power_mw(out->pack_p, sizeof(out->pack_p), mw);
      }
    }
    {
      char t0[12], t1[12], t2[12];
      psu_format_centi_c(t0, sizeof(t0), frame->temp_centi[0]);
      psu_format_centi_c(t1, sizeof(t1), frame->temp_centi[1]);
      psu_format_centi_c(t2, sizeof(t2), frame->temp_centi[2]);
      (void)snprintf(out->temp, sizeof(out->temp), "%s  %s  %s", t0, t1, t2);
    }
    copy_text(out->fet_chg, sizeof(out->fet_chg), frame->fet_chg ? "ON" : "OFF");
    copy_text(out->fet_dsg, sizeof(out->fet_dsg), frame->fet_dsg ? "ON" : "OFF");
    if (frame->balance_mask == 0U)
      copy_text(out->balance, sizeof(out->balance), "OFF");
    else
    {
      size_t used = 0U;
      out->balance[0] = '\0';
      for (i = 0; i < 4U; ++i)
      {
        int wrote;
        if ((frame->balance_mask & (1U << i)) == 0U)
          continue;
        wrote = snprintf(out->balance + used, sizeof(out->balance) - used, "%sCH%u",
                         used ? " " : "", i + 1U);
        if (wrote < 0)
          break;
        used += (size_t)wrote;
      }
    }
    copy_text(out->fault, sizeof(out->fault), frame->fault[0] ? frame->fault : "OK");
  }
  (void)snprintf(out->summary, sizeof(out->summary),
                 "SOC %s\nPACK %s  %s  %s\nSPREAD %s  T %s\nBAL %s  %s",
                 out->soc, out->pack_v, out->pack_i, out->pack_p,
                 out->spread, out->temp, out->balance, out->fault);
}
