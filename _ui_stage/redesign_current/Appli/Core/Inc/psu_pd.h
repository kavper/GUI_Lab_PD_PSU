#ifndef PSU_PD_H
#define PSU_PD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* USB PD 3.1 source PDO / RDO bitfields.
   This repository has no G4 return grammar, so the product path passes
   a zero count and must not invent rows. Missing on the wire:
   PDO/APDO words, selected object, RDO (index, request, flags),
   partner port role, negotiation state, requested contract,
   confirmed contract, port voltage, port current, port power. */

#define PSU_PD_MAX 7u

#define PSU_PD_UNAVAILABLE \
  "PDO/APDO list unavailable.\n" \
  "Missing from G4: PDO words, selected object,\n" \
  "RDO, partner role, contract V/I, port V/I/P.\n" \
  "No PDO rows are shown."

typedef struct
{
  uint8_t type; /* 0 none, 1 fixed, 2 variable, 3 battery, 4 APDO */
  uint8_t apdo_kind;
  uint16_t vmin_mv;
  uint16_t vmax_mv;
  uint16_t imax_ma;
  uint32_t pmax_mw;
} PsuPdo;

typedef struct
{
  uint8_t valid;
  uint8_t pps;
  uint8_t object; /* 1..7 */
  uint8_t giveback;
  uint8_t mismatch;
  uint8_t usb_comms;
  uint8_t no_suspend;
  uint8_t unchunked;
  uint16_t voltage_mv;
  uint16_t operating_ma;
  uint16_t max_ma;
} PsuRdo;

typedef struct
{
  uint8_t count;
  int8_t selected; /* -1, else 0-based index into line[] */
  uint8_t list_valid;
  uint8_t rdo_valid;
  char line[PSU_PD_MAX][48];
  char block[320];
  char rdo[96];
} PsuPdText;

int psu_pd_decode_pdo(uint32_t word, PsuPdo *out);
int psu_pd_decode_rdo(uint32_t word, int pps, PsuRdo *out);
void psu_pd_describe(const uint32_t *words, uint8_t count, int have_rdo,
                     uint32_t rdo_word, PsuPdText *out);

#ifdef __cplusplus
}
#endif

#endif
