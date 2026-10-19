#include "psu_pd.h"
#include <stdio.h>
#include <string.h>

static void put_mv(char *dst, size_t n, uint32_t mv)
{
  if (mv < 10000U)
    (void)snprintf(dst, n, "%u.%03u", (unsigned)(mv / 1000U), (unsigned)(mv % 1000U));
  else
    (void)snprintf(dst, n, "%u.%02u", (unsigned)(mv / 1000U), (unsigned)((mv % 1000U) / 10U));
}

static void put_ma(char *dst, size_t n, uint32_t ma)
{
  (void)snprintf(dst, n, "%u.%03u", (unsigned)(ma / 1000U), (unsigned)(ma % 1000U));
}

int psu_pd_decode_pdo(uint32_t word, PsuPdo *out)
{
  uint32_t kind;
  if (out == 0 || word == 0U)
    return 0;
  memset(out, 0, sizeof(*out));
  kind = (word >> 30) & 0x3U;
  if (kind == 0U)
  {
    out->type = 1U;
    out->vmax_mv = (uint16_t)((((word >> 10) & 0x3FFU) * 50U));
    out->vmin_mv = out->vmax_mv;
    out->imax_ma = (uint16_t)((word & 0x3FFU) * 10U);
    return out->vmax_mv != 0U;
  }
  if (kind == 1U)
  {
    out->type = 2U;
    out->vmax_mv = (uint16_t)((((word >> 20) & 0x3FFU) * 50U));
    out->vmin_mv = (uint16_t)((((word >> 10) & 0x3FFU) * 50U));
    out->imax_ma = (uint16_t)((word & 0x3FFU) * 10U);
    return 1;
  }
  if (kind == 2U)
  {
    out->type = 3U;
    out->vmax_mv = (uint16_t)((((word >> 20) & 0x3FFU) * 50U));
    out->vmin_mv = (uint16_t)((((word >> 10) & 0x3FFU) * 50U));
    out->pmax_mw = (word & 0x3FFU) * 250U;
    return 1;
  }
  out->type = 4U;
  out->apdo_kind = (uint8_t)((word >> 28) & 0x3U);
  if (out->apdo_kind != 0U)
    return 1;
  out->vmax_mv = (uint16_t)((((word >> 17) & 0xFFU) * 100U));
  out->vmin_mv = (uint16_t)((((word >> 8) & 0xFFU) * 100U));
  out->imax_ma = (uint16_t)((word & 0x7FU) * 50U);
  return 1;
}

int psu_pd_decode_rdo(uint32_t word, int pps, PsuRdo *out)
{
  uint32_t object;
  if (out == 0 || word == 0U)
    return 0;
  memset(out, 0, sizeof(*out));
  object = (word >> 28) & 0x7U;
  if (object < 1U || object > 7U)
    return 0;
  out->valid = 1U;
  out->object = (uint8_t)object;
  out->pps = pps ? 1U : 0U;
  out->mismatch = (uint8_t)((word >> 26) & 1U);
  out->usb_comms = (uint8_t)((word >> 25) & 1U);
  out->no_suspend = (uint8_t)((word >> 24) & 1U);
  out->unchunked = (uint8_t)((word >> 23) & 1U);
  if (pps)
  {
    out->voltage_mv = (uint16_t)((((word >> 9) & 0xFFFU) * 20U));
    out->operating_ma = (uint16_t)((word & 0x7FU) * 50U);
  }
  else
  {
    out->giveback = (uint8_t)((word >> 27) & 1U);
    out->operating_ma = (uint16_t)((((word >> 10) & 0x3FFU) * 10U));
    out->max_ma = (uint16_t)((word & 0x3FFU) * 10U);
  }
  return 1;
}

static void format_pdo_line(char *dst, size_t n, unsigned index, int selected, const PsuPdo *pdo)
{
  char vmin[12];
  char vmax[12];
  char imax[12];
  const char *mark = selected ? ">" : " ";
  const char *name = "PDO";
  put_mv(vmin, sizeof(vmin), pdo->vmin_mv);
  put_mv(vmax, sizeof(vmax), pdo->vmax_mv);
  put_ma(imax, sizeof(imax), pdo->imax_ma);
  if (pdo->type == 1U)
    name = "FIXED";
  else if (pdo->type == 2U)
    name = "VAR";
  else if (pdo->type == 3U)
    name = "BATT";
  else if (pdo->type == 4U)
    name = "APDO";
  if (pdo->type == 4U && pdo->apdo_kind != 0U)
  {
    (void)snprintf(dst, n, "%s%u APDO KIND %u", mark, index, pdo->apdo_kind);
    return;
  }
  if (pdo->type == 3U)
  {
    (void)snprintf(dst, n, "%s%u BATT %s-%s V  %u.%02u W",
                   mark, index, vmin, vmax,
                   (unsigned)(pdo->pmax_mw / 1000U),
                   (unsigned)((pdo->pmax_mw % 1000U) / 10U));
    return;
  }
  if (pdo->type == 1U)
  {
    (void)snprintf(dst, n, "%s%u %s %s V  %s A", mark, index, name, vmax, imax);
    return;
  }
  (void)snprintf(dst, n, "%s%u %s %s-%s V  %s A", mark, index, name, vmin, vmax, imax);
}

static void format_rdo(char *dst, size_t n, const PsuRdo *rdo)
{
  char volts[12];
  char ops[12];
  char mx[12];
  char flags[48];
  size_t used = 0U;
  flags[0] = '\0';
  if (rdo->mismatch)
    used += (size_t)snprintf(flags + used, sizeof(flags) - used, " MISMATCH");
  if (rdo->giveback)
    used += (size_t)snprintf(flags + used, sizeof(flags) - used, " GIVEBACK");
  if (rdo->usb_comms)
    used += (size_t)snprintf(flags + used, sizeof(flags) - used, " USBCOM");
  if (rdo->no_suspend)
    used += (size_t)snprintf(flags + used, sizeof(flags) - used, " NOSUSP");
  if (rdo->unchunked)
    used += (size_t)snprintf(flags + used, sizeof(flags) - used, " UNCHUNK");
  if (rdo->pps)
  {
    put_mv(volts, sizeof(volts), rdo->voltage_mv);
    put_ma(ops, sizeof(ops), rdo->operating_ma);
    (void)snprintf(dst, n, "OBJ %u  %s V  %s A%s", rdo->object, volts, ops, flags);
    return;
  }
  put_ma(ops, sizeof(ops), rdo->operating_ma);
  put_ma(mx, sizeof(mx), rdo->max_ma);
  (void)snprintf(dst, n, "OBJ %u  OP %s A  MAX %s A%s", rdo->object, ops, mx, flags);
}

void psu_pd_describe(const uint32_t *words, uint8_t count, int have_rdo,
                     uint32_t rdo_word, PsuPdText *out)
{
  PsuPdo pdo;
  PsuRdo rdo;
  unsigned i;
  int pps = 0;
  int selected = -1;
  size_t used = 0U;
  if (out == 0)
    return;
  memset(out, 0, sizeof(*out));
  out->selected = -1;
  memset(&rdo, 0, sizeof(rdo));
  if (count > PSU_PD_MAX)
    count = PSU_PD_MAX;
  if (words == 0)
    count = 0U;
  if (have_rdo && psu_pd_decode_rdo(rdo_word, 0, &rdo))
  {
    if (rdo.object >= 1U && rdo.object <= count &&
        psu_pd_decode_pdo(words[rdo.object - 1U], &pdo) && pdo.type == 4U && pdo.apdo_kind == 0U)
    {
      pps = 1;
      (void)psu_pd_decode_rdo(rdo_word, pps, &rdo);
    }
    if (rdo.object >= 1U && (uint8_t)rdo.object <= count)
      selected = (int)rdo.object - 1;
  }
  else if (have_rdo)
  {
    (void)snprintf(out->rdo, sizeof(out->rdo), "RDO PRESENT BUT NOT DECODED");
  }
  for (i = 0; i < count; ++i)
  {
    if (!psu_pd_decode_pdo(words[i], &pdo))
      continue;
    format_pdo_line(out->line[out->count], sizeof(out->line[out->count]),
                    out->count + 1U, (int)i == selected, &pdo);
    if ((int)i == selected)
      out->selected = (int8_t)out->count;
    out->count++;
  }
  if (out->count == 0U)
  {
    (void)snprintf(out->block, sizeof(out->block), "%s", PSU_PD_UNAVAILABLE);
    (void)snprintf(out->rdo, sizeof(out->rdo), "%s",
                   have_rdo ? "RDO PRESENT BUT PDO LIST MISSING" : "UNAVAILABLE");
    return;
  }
  out->list_valid = 1U;
  out->block[0] = '\0';
  for (i = 0; i < out->count; ++i)
  {
    int wrote = snprintf(out->block + used, sizeof(out->block) - used, "%s%s",
                         i ? "\n" : "", out->line[i]);
    if (wrote < 0 || (size_t)wrote >= sizeof(out->block) - used)
      break;
    used += (size_t)wrote;
  }
  if (have_rdo && rdo.valid)
  {
    out->rdo_valid = 1U;
    format_rdo(out->rdo, sizeof(out->rdo), &rdo);
  }
  else if (out->rdo[0] == '\0')
  {
    (void)snprintf(out->rdo, sizeof(out->rdo), "UNAVAILABLE");
  }
}
