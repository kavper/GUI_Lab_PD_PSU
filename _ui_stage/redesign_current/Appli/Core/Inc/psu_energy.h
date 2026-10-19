#ifndef PSU_ENERGY_H
#define PSU_ENERGY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Gap longer than this is not integrated. Same order as the G4 stale window. */
#define PSU_ENERGY_GAP_MS 1500u

typedef struct
{
  uint8_t active;
  uint8_t connected;
  uint8_t have_sample;
  uint8_t in_gap;
  uint8_t last_dir; /* 0 none, 1 in, 2 out */
  uint8_t has_closed;
  uint32_t live_ms;
  uint32_t gap_ms;
  uint32_t gap_count;
  uint32_t last_ms;
  uint32_t in_mwh;
  uint32_t out_mwh;
  uint64_t in_resid;
  uint64_t out_resid;
  uint32_t closed_live_ms;
  uint32_t closed_in_mwh;
  uint32_t closed_out_mwh;
  uint32_t closed_gaps;
} PsuEnergySession;

void psu_energy_reset(PsuEnergySession *s);

/* current_ma > 0 is current leaving the instrument (energy out).
   current_ma < 0 is current entering (energy in).
   A gap, a disconnect, or the first sample does not invent watt-hours. */
void psu_energy_observe(PsuEnergySession *s, uint32_t now_ms, int connected,
                        int sample_valid, int32_t power_mw);

void psu_energy_format(const PsuEnergySession *s, char *dst, size_t n);

#ifdef __cplusplus
}
#endif

#endif
