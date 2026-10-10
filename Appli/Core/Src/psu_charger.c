#include "psu_charger.h"
#include "psu_limits.h"
#include <stdio.h>
#include <string.h>

const char *psu_chg_state_name(PsuChgState state)
{
  switch (state)
  {
  case CHG_IDLE: return "IDLE";
  case CHG_VALIDATE: return "VALIDATE";
  case CHG_STARTING: return "WAITING FOR G4 / G0 START";
  case CHG_WAIT: return "WAIT_FOR_CONNECTION";
  case CHG_PRECHARGE: return "PRECHARGE";
  case CHG_CC: return "CONSTANT_CURRENT";
  case CHG_CV: return "CONSTANT_VOLTAGE";
  case CHG_ABSORPTION: return "ABSORPTION";
  case CHG_FLOAT: return "FLOAT";
  case CHG_TERMINATING: return "TERMINATING";
  case CHG_COMPLETE: return "COMPLETE";
  case CHG_PAUSED: return "PAUSED";
  case CHG_ABORTED: return "ABORTED";
  case CHG_FAULT: return "FAULT";
  default: return "FAULT";
  }
}

void psu_chg_profile_defaults(PsuChgProfile *profile, uint8_t chemistry)
{
  if (profile == 0)
    return;
  memset(profile, 0, sizeof(*profile));
  profile->chemistry = chemistry;
  profile->cells = 1U;
  profile->capacity_mah = 2000U;
  profile->c_rate_milli = 500U;
  profile->phase_hold_ms = 500U;
  profile->max_session_ms = 4U * 3600U * 1000U;
  /* Legacy field names; all controller temperatures use 0.1 C (10..45 C). */
  profile->temp_min_centi = 100;
  profile->temp_max_centi = 450;
  switch (chemistry)
  {
  case CHEM_LIION_HV:
    profile->precharge_mv_cell = 3000U;
    profile->cv_mv_cell = 4350U;
    profile->precharge_ma = 100U;
    profile->cc_ma = 500U;
    profile->term_ma = 100U;
    break;
  case CHEM_LIFEPO4:
    profile->precharge_mv_cell = 2500U;
    profile->cv_mv_cell = 3550U;
    profile->precharge_ma = 100U;
    profile->cc_ma = 500U;
    profile->term_ma = 100U;
    break;
  case CHEM_LTO:
    profile->precharge_mv_cell = 1800U;
    profile->cv_mv_cell = 2700U;
    profile->precharge_ma = 100U;
    profile->cc_ma = 500U;
    profile->term_ma = 100U;
    break;
  case CHEM_LEAD:
    profile->precharge_mv_cell = 2000U;
    profile->cv_mv_cell = 2400U;
    profile->float_mv_cell = 2250U;
    profile->precharge_ma = 100U;
    profile->cc_ma = 500U;
    profile->term_ma = 100U;
    break;
  case CHEM_NIMH:
    profile->cv_mv_cell = 1500U;
    break;
  case CHEM_CUSTOM:
    profile->precharge_mv_cell = 3000U;
    profile->cv_mv_cell = 4200U;
    profile->precharge_ma = 100U;
    profile->cc_ma = 200U;
    profile->term_ma = 50U;
    break;
  default:
    profile->chemistry = CHEM_LIION;
    profile->precharge_mv_cell = 3000U;
    profile->cv_mv_cell = 4200U;
    profile->precharge_ma = 100U;
    profile->cc_ma = 500U;
    profile->term_ma = 100U;
    break;
  }
}

uint32_t psu_chg_target_mv(const PsuChgProfile *p){return p->target_pack_mv?p->target_pack_mv:(uint32_t)p->cells*p->cv_mv_cell;}
int psu_chg_set_target(PsuChgProfile *p,uint32_t mv){
  uint32_t hi=(uint32_t)p->cells*p->cv_mv_cell;
  uint32_t lo=(uint32_t)p->cells*p->precharge_mv_cell;
  if(!mv||mv<lo||mv>hi||mv>PSU_VOLTAGE_MAX_MV)return 0;
  p->target_pack_mv=mv;return 1;
}
uint8_t psu_chg_max_cells(const PsuChgProfile *profile)
{
  uint32_t n;
  if (profile == 0 || profile->cv_mv_cell == 0U)
    return 0U;
  n = PSU_VOLTAGE_MAX_MV / profile->cv_mv_cell;
  if (n > 8U)
    n = 8U;
  return (uint8_t)n;
}

void psu_chg_init(PsuCharger *chg)
{
  if (chg == 0)
    return;
  memset(chg, 0, sizeof(*chg));
  psu_chg_profile_defaults(&chg->profile, CHEM_LIION);
  chg->state = CHG_IDLE;
  (void)snprintf(chg->reason, sizeof(chg->reason), "IDLE");
}

static uint32_t c_rate_ma(const PsuChgProfile *p)
{
  return (uint32_t)(((uint64_t)p->capacity_mah * p->c_rate_milli) / 1000U);
}

int psu_chg_validate(const PsuChgProfile *profile, const PsuChgSense *sense, char *why, unsigned why_n)
{
  uint32_t pack_cv;
  uint32_t limit_ma;
  if (why && why_n)
    why[0] = '\0';
  if (profile == 0)
    return 0;
  if (profile->chemistry == CHEM_NIMH)
  {
    if (why) (void)snprintf(why, why_n, "NiMH unsupported without proven dV/dt");
    return 0;
  }
  if (profile->chemistry == CHEM_CUSTOM && !profile->custom_unlocked)
  {
    if (why) (void)snprintf(why, why_n, "Custom profile needs explicit unlock");
    return 0;
  }
  if (profile->chemistry == CHEM_LIION_HV && profile->cv_mv_cell < 4200U)
  {
    if (why) (void)snprintf(why, why_n, "LiHV requires an explicit CV choice");
    return 0;
  }
  if (!profile->confirmed)
  {
    if (why) (void)snprintf(why, why_n, "Confirmation required");
    return 0;
  }
  if (profile->cells < 1U || profile->cells > 8U)
  {
    if (why) (void)snprintf(why, why_n, "Cell count");
    return 0;
  }
  pack_cv = psu_chg_target_mv(profile);
  if(profile->target_pack_mv && (pack_cv>(uint32_t)profile->cells*profile->cv_mv_cell || pack_cv<(uint32_t)profile->cells*profile->precharge_mv_cell)){if(why)snprintf(why,why_n,"Target outside chemistry range");return 0;}
  if (pack_cv > PSU_VOLTAGE_MAX_MV || profile->cv_mv_cell == 0U)
  {
    if (why) (void)snprintf(why, why_n, "Pack voltage exceeds 27 V");
    return 0;
  }
  limit_ma = c_rate_ma(profile);
  if (profile->cc_ma == 0U || profile->cc_ma > PSU_CURRENT_MAX_MA ||
      (limit_ma != 0U && profile->cc_ma > limit_ma))
  {
    if (why) (void)snprintf(why, why_n, "Current exceeds C-rate or 5 A");
    return 0;
  }
  if (sense == 0 || !sense->telemetry_ok || (!sense->permit && !sense->start_allowed))
  {
    if (why) (void)snprintf(why, why_n, "Telemetry or permit missing");
    return 0;
  }
  if(sense->pack_mv>pack_cv+50U){if(why)snprintf(why,why_n,"Pack above target - no charge");return 0;}
  if (sense->reverse_polarity)
  {
    if (why) (void)snprintf(why, why_n, "Reverse polarity");
    return 0;
  }
  if (!sense->reverse_hw && !profile->polarity_checked)
  {
    if (why) (void)snprintf(why, why_n, "Manual polarity check required");
    return 0;
  }
  return 1;
}

static void fault(PsuCharger *chg, const char *reason)
{
  chg->state = CHG_FAULT;
  chg->running = 0U;
  (void)snprintf(chg->reason, sizeof(chg->reason), "%s", reason);
  if (chg->io.output)
    chg->io.output(0, chg->io.user);
}

void psu_chg_abort(PsuCharger *chg, const char *reason, uint32_t now_ms)
{
  if (chg == 0)
    return;
  if(chg->running)chg->elapsed_ms=now_ms-chg->session_start_ms;
  chg->state_since_ms = now_ms;
  fault(chg, reason ? reason : "ABORTED");
  chg->state = CHG_ABORTED;
}

int psu_chg_start(PsuCharger *chg, const PsuChgProfile *profile, const PsuChgSense *sense, uint32_t now_ms)
{
  char why[64];
  if (chg == 0 || profile == 0)
    return 0;
  if (!psu_chg_validate(profile, sense, why, sizeof(why)))
  {
    (void)snprintf(chg->reason, sizeof(chg->reason), "%s", why);
    return 0;
  }
  chg->profile = *profile;
  chg->state = CHG_VALIDATE;
  chg->state_since_ms = now_ms;
  chg->hold_since_ms = now_ms;
  chg->running = 1U;
  chg->delivered_mah = 0U;
  chg->delivered_mwh = 0U;
  chg->last_sample_ms = now_ms;
  chg->session_start_ms = now_ms;chg->elapsed_ms=0;
  chg->charge_remainder=chg->energy_remainder=0;
  chg->trace_count=1;chg->trace_time[0]=0;chg->trace_mv[0]=sense->pack_mv;chg->trace_ma[0]=sense->current_ma>0?(uint32_t)sense->current_ma:0;
  chg->trace_last_ms=now_ms;chg->trace_interval_ms=1000;
  chg->command_valid=chg->output_started=0;
  chg->min_temp = sense->temp_centi;
  chg->max_temp = sense->temp_centi;
  (void)snprintf(chg->reason, sizeof(chg->reason), "VALIDATE");
  return 1;
}

int psu_chg_user_start(PsuCharger *chg, PsuChgProfile *profile, const PsuChgSense *sense,
                       uint32_t now_ms, int chemistry_set, int cells_set, int polarity_already)
{
  uint8_t max_cells;
  if (chg == 0 || profile == 0)
    return 0;
  if (chg->running)
  {
    (void)snprintf(chg->reason, sizeof(chg->reason), "ALREADY RUNNING");
    return 0;
  }
  if (!chemistry_set)
  {
    (void)snprintf(chg->reason, sizeof(chg->reason), "SELECT A PROFILE");
    return 0;
  }
  max_cells = psu_chg_max_cells(profile);
  if (!cells_set || profile->cells < 1U || profile->cells > max_cells)
  {
    (void)snprintf(chg->reason, sizeof(chg->reason), "CELL COUNT REQUIRED");
    return 0;
  }
  if (!polarity_already)
  {
    profile->polarity_checked = 1U;
    profile->confirmed = 0U;
    chg->profile = *profile;
    (void)snprintf(chg->reason, sizeof(chg->reason), "POLARITY CONFIRMED. PRESS START");
    return 1;
  }
  profile->confirmed = 1U;
  profile->polarity_checked = 1U;
  if (profile->chemistry == CHEM_CUSTOM)
    profile->custom_unlocked = 1U;
  if (!psu_chg_start(chg, profile, sense, now_ms))
    return 0;
  return 2;
}

static int held(PsuCharger *chg, int condition, uint32_t now_ms)
{
  if (!condition)
  {
    chg->hold_since_ms = now_ms;
    return 0;
  }
  return (now_ms - chg->hold_since_ms) >= chg->profile.phase_hold_ms;
}

static void enter(PsuCharger *chg, PsuChgState state, uint32_t now_ms)
{
  chg->state = state;
  chg->state_since_ms = now_ms;
  chg->hold_since_ms = now_ms;
  (void)snprintf(chg->reason, sizeof(chg->reason), "%s", psu_chg_state_name(state));
}

static void send_limits(PsuCharger *chg,uint32_t mv,uint32_t ma) {
  if(!chg->command_valid||chg->command_mv!=mv||chg->command_ma!=ma) {
    if(chg->io.limits)chg->io.limits(mv,ma,chg->io.user);
    chg->command_mv=mv;chg->command_ma=ma;chg->command_valid=1;
  }
}
static void trace_sample(PsuCharger *chg,const PsuChgSense *sense,uint32_t now_ms) {
  unsigned i;
  if(now_ms-chg->trace_last_ms<chg->trace_interval_ms)return;
  if(chg->trace_count==128) {
    for(i=1;i<64;i++){chg->trace_time[i]=chg->trace_time[i*2];chg->trace_mv[i]=chg->trace_mv[i*2];chg->trace_ma[i]=chg->trace_ma[i*2];}
    chg->trace_count=64;chg->trace_interval_ms*=2;
  }
  i=chg->trace_count++;chg->trace_time[i]=chg->elapsed_ms;chg->trace_mv[i]=sense->pack_mv;chg->trace_ma[i]=sense->current_ma>0?(uint32_t)sense->current_ma:0;
  chg->trace_last_ms=now_ms;
}
void psu_chg_tick(PsuCharger *chg, const PsuChgSense *sense, uint32_t now_ms)
{
  uint32_t pack_cv;
  uint32_t pre_mv;
  if (chg == 0 || !chg->running)
    return;
  chg->elapsed_ms=now_ms-chg->session_start_ms;
  if (sense == 0 || !sense->telemetry_ok)
  {
    fault(chg, "Telemetry lost");
    return;
  }
  if (chg->state != CHG_VALIDATE && chg->state != CHG_WAIT && chg->state != CHG_STARTING && (!sense->permit || !sense->output_ready)) {
    fault(chg,"G4 output / PERMIT lost");return;
  }
  if (sense->temp_centi < chg->min_temp)
    chg->min_temp = sense->temp_centi;
  if (sense->temp_centi > chg->max_temp)
    chg->max_temp = sense->temp_centi;
  if(sense->current_ma>0) {
    const uint32_t dt=now_ms-chg->last_sample_ms;
    chg->charge_remainder+=(uint64_t)sense->current_ma*dt;
    chg->energy_remainder+=(uint64_t)sense->pack_mv*(uint32_t)sense->current_ma*dt;
    chg->delivered_mah+=(uint32_t)(chg->charge_remainder/3600000U);chg->charge_remainder%=3600000U;
    chg->delivered_mwh+=(uint32_t)(chg->energy_remainder/3600000000ULL);chg->energy_remainder%=3600000000ULL;
  }
  trace_sample(chg,sense,now_ms);
  chg->last_sample_ms = now_ms;
  if (chg->profile.temp_sensor &&
      (sense->temp_centi < chg->profile.temp_min_centi ||
       sense->temp_centi > chg->profile.temp_max_centi))
  {
    fault(chg, "Temperature limit");
    return;
  }
  if (chg->elapsed_ms > chg->profile.max_session_ms)
  {
    fault(chg, "Session timeout");
    return;
  }
  pack_cv = psu_chg_target_mv(&chg->profile);
  pre_mv = (uint32_t)chg->profile.cells * chg->profile.precharge_mv_cell;
  if (chg->state == CHG_VALIDATE || chg->state == CHG_WAIT) {
    send_limits(chg,pack_cv,chg->profile.precharge_ma);
    if(!chg->running)return;
    enter(chg,CHG_STARTING,now_ms);
    return;
  }
  if (chg->state == CHG_STARTING) {
    if (!chg->output_started && sense->limits_applied && chg->io.output) {
      chg->output_started=1;chg->io.output(1,chg->io.user);
      if(!chg->running)return;
    }
    if(chg->output_started && sense->permit && sense->output_ready)enter(chg,CHG_PRECHARGE,now_ms);
    else if(now_ms-chg->state_since_ms>5000U)fault(chg,"G4/G0 start timeout");
    return;
  }
  if (chg->state == CHG_PRECHARGE)
  {
    send_limits(chg,pack_cv,chg->profile.precharge_ma);
    if (held(chg, sense->pack_mv >= pre_mv, now_ms))
      enter(chg, CHG_CC, now_ms);
    return;
  }
  if (chg->state == CHG_CC)
  {
    send_limits(chg,pack_cv,chg->profile.cc_ma);
    if (held(chg, sense->pack_mv + 50U >= pack_cv, now_ms))
      enter(chg, (chg->profile.chemistry == CHEM_LEAD) ? CHG_ABSORPTION : CHG_CV, now_ms);
    return;
  }
  if (chg->state == CHG_CV || chg->state == CHG_ABSORPTION)
  {
    send_limits(chg,pack_cv,chg->profile.cc_ma);
    if (held(chg, sense->current_ma >= 0 && (uint32_t)sense->current_ma <= chg->profile.term_ma, now_ms))
      enter(chg, CHG_TERMINATING, now_ms);
    return;
  }
  if (chg->state == CHG_TERMINATING)
  {
    if (chg->profile.chemistry == CHEM_LEAD && chg->profile.float_mv_cell != 0U)
    {
      uint32_t fv = (uint32_t)chg->profile.cells * chg->profile.float_mv_cell;
      if(fv>pack_cv)fv=pack_cv;
      send_limits(chg,fv,chg->profile.term_ma);
      enter(chg, CHG_FLOAT, now_ms);
      return;
    }
    if (chg->io.output)
      chg->io.output(0, chg->io.user);
    chg->running = 0U;
    enter(chg, CHG_COMPLETE, now_ms);
    return;
  }
}
