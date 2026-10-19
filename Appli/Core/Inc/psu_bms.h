#ifndef PSU_BMS_H
#define PSU_BMS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Physical channels stay CH1..CH4. An absent channel is not 0 V and
   does not shift the others. */

typedef struct
{
  uint8_t valid;
  uint8_t present[4];
  uint8_t stale[4];
  uint16_t cell_mv[4];
  int16_t temp_centi[3];
  uint8_t fet_chg;
  uint8_t fet_dsg;
  uint8_t balance_mask; /* bit 0 = CH1 */
  uint8_t soc_valid;
  uint8_t soc_pct;
  uint8_t current_valid;
  int32_t current_ma;
  char fault[40];
} PsuBmsFrame;

typedef struct
{
  char state[16];
  char soc[20];
  char pack_v[20];
  char pack_i[20];
  char pack_p[20];
  char spread[16];
  char temp[48];
  char balance[20];
  char fet_chg[8];
  char fet_dsg[8];
  char fault[40];
  char channel[4][16];
  char summary[180];
  char channels_line[80];
} PsuBmsFace;

void psu_bms_face(const PsuBmsFrame *frame, PsuBmsFace *out);

#ifdef __cplusplus
}
#endif

#endif
