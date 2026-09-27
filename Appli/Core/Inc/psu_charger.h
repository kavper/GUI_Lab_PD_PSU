#ifndef PSU_CHARGER_H
#define PSU_CHARGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  CHG_IDLE = 0,
  CHG_VALIDATE,
  CHG_WAIT,
  CHG_PRECHARGE,
  CHG_CC,
  CHG_CV,
  CHG_ABSORPTION,
  CHG_FLOAT,
  CHG_TERMINATING,
  CHG_COMPLETE,
  CHG_PAUSED,
  CHG_ABORTED,
  CHG_FAULT
} PsuChgState;

typedef enum
{
  CHEM_LIION = 0,
  CHEM_LIION_HV = 1,
  CHEM_LIFEPO4 = 2,
  CHEM_LTO = 3,
  CHEM_LEAD = 4,
  CHEM_NIMH = 5,
  CHEM_CUSTOM = 6
} PsuChem;

typedef struct
{
  uint8_t chemistry;
  uint8_t cells;
  uint32_t capacity_mah;
  uint32_t c_rate_milli; /* 500 = 0.5C */
  uint32_t precharge_mv_cell;
  uint32_t precharge_ma;
  uint32_t cc_ma;
  uint32_t cv_mv_cell;
  uint32_t term_ma;
  uint32_t float_mv_cell;
  uint32_t max_session_ms;
  uint32_t phase_hold_ms;
  int16_t temp_min_centi;
  int16_t temp_max_centi;
  uint8_t temp_sensor;
  uint8_t remote_sense;
  uint8_t confirmed;
  uint8_t custom_unlocked;
  uint8_t polarity_checked;
} PsuChgProfile;

typedef struct
{
  void (*limits)(uint32_t mv, uint32_t ma, void *user);
  void (*output)(int on, void *user);
  void (*permit)(int on, void *user);
  void *user;
} PsuChgIo;

typedef struct
{
  uint32_t pack_mv;
  int32_t current_ma;
  int16_t temp_centi;
  uint8_t telemetry_ok;
  uint8_t permit;
  uint8_t reverse_polarity; /* 1 only if hardware reported it */
  uint8_t reverse_hw;
} PsuChgSense;

typedef struct
{
  PsuChgProfile profile;
  PsuChgState state;
  uint32_t state_since_ms;
  uint32_t hold_since_ms;
  uint32_t delivered_mah;
  uint32_t delivered_mwh;
  uint32_t last_sample_ms;
  int32_t min_temp;
  int32_t max_temp;
  char reason[64];
  uint8_t running;
  PsuChgIo io;
} PsuCharger;

typedef struct
{
  uint32_t uptime_ms;
  uint8_t chemistry;
  uint8_t cells;
  uint32_t seconds;
  uint32_t mah;
  uint32_t mwh;
  uint8_t result;
  char reason[40];
} PsuChgRecord;

void psu_chg_init(PsuCharger *chg);
void psu_chg_profile_defaults(PsuChgProfile *profile, uint8_t chemistry);
int psu_chg_validate(const PsuChgProfile *profile, const PsuChgSense *sense, char *why, unsigned why_n);
int psu_chg_start(PsuCharger *chg, const PsuChgProfile *profile, const PsuChgSense *sense, uint32_t now_ms);
void psu_chg_tick(PsuCharger *chg, const PsuChgSense *sense, uint32_t now_ms);
void psu_chg_abort(PsuCharger *chg, const char *reason, uint32_t now_ms);
const char *psu_chg_state_name(PsuChgState state);

#ifdef __cplusplus
}
#endif

#endif
