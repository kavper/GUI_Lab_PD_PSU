#include "psu_app.h"
#include "psu_limits.h"
#include "psu_store.h"
#include <stdio.h>
#include <string.h>

#define CMD_RING 8

typedef struct
{
  uint16_t id;
  uint8_t state;
  uint8_t source;
  uint32_t mv;
  uint32_t ma;
  uint32_t sent_ms;
  uint8_t track_readback;
  char name[16];
} PsuCmdRec;

typedef struct
{
  uint32_t schema;
  PsuPreset presets[8]; /* Keep persisted layout compatible; only three are exposed. */
  uint8_t preset_count;
  PsuSeqStep steps[PSU_SEQ_MAX_STEPS];
  uint8_t step_count;
  uint8_t seq_mode;
  uint16_t seq_loops;
  uint32_t ovp_mv;
  uint32_t ocp_ma;
  uint8_t output_on;
} PsuBlob;

static PsuHalHooks hooks;
static uint8_t app_ready;
static uint32_t app_now;
static G4Port g4;
static PsuSequencer sequencer;
static PsuCharger charger;
static PsuSnapshot snap;
static PsuG0Sample g0;
static PsuCmdRec cmds[CMD_RING];
static uint8_t cmd_count;
static uint16_t next_cmd = 1;
static volatile uint32_t generation;
static int button_level;
static uint32_t button_change_ms;
static int button_stable;
static PsuBlob blob;
static uint8_t remote_access = 1;
static PsuStore store;
static uint8_t *store_a;
static uint8_t *store_b;
static size_t store_n;
static char lease_client[24];
static uint8_t g0_seen;
static uint32_t host_serial, host_tx_count;
static uint8_t clear_ack_pending;
static uint32_t clear_ack_serial, startup_ms, stop_ms;
static uint32_t energy_ms;
static uint8_t energy_seen;
static char web_page_storage[4096];

static void touch(void)
{
  ++generation;
  if ((generation & 1U) == 0U)
    ++generation;
  snap.revision = generation;
}

static void publish_end(void)
{
  if (generation & 1U)
    ++generation;
  snap.revision = generation;
}

static void fill_defaults(void)
{
  memset(&blob, 0, sizeof(blob));
  blob.schema = PSU_STORE_SCHEMA;
  blob.preset_count = 3U;
  snprintf(blob.presets[0].name, sizeof(blob.presets[0].name), "5V 1A");
  blob.presets[0].voltage_mv = 5000U;
  blob.presets[0].current_ma = 1000U;
  snprintf(blob.presets[1].name, sizeof(blob.presets[1].name), "12V 2A");
  blob.presets[1].voltage_mv = 12000U;
  blob.presets[1].current_ma = 2000U;
  snprintf(blob.presets[2].name, sizeof(blob.presets[2].name), "20V 3A");
  blob.presets[2].voltage_mv = 20000U;
  blob.presets[2].current_ma = 3000U;
  blob.step_count = 1U;
  blob.steps[0].voltage_mv = 5000U;
  blob.steps[0].current_ma = 1000U;
  blob.steps[0].time_ms = 1000U;
  blob.steps[0].enabled = 1U;
  blob.ovp_mv = PSU_VOLTAGE_MAX_MV;
  blob.ocp_ma = PSU_CURRENT_MAX_MA;
  blob.output_on = 0U;
  blob.seq_mode = PSU_SEQ_ONCE;
  blob.seq_loops = 1U;
}

static void apply_blob_to_runtime(void)
{
  uint8_t i;
  blob.output_on = 0U;
  for (i = 0; i < blob.step_count && i < PSU_SEQ_MAX_STEPS; ++i)
    sequencer.steps[i] = blob.steps[i];
  sequencer.count = blob.step_count ? blob.step_count : 1U;
  sequencer.mode = blob.seq_mode;
  sequencer.loops_requested = blob.seq_loops ? blob.seq_loops : 1U;
}

static uint16_t push_cmd(const char *name, uint8_t source, uint32_t mv, uint32_t ma, int track)
{
  PsuCmdRec *rec;
  if (cmd_count < CMD_RING)
    rec = &cmds[cmd_count++];
  else
  {
    memmove(&cmds[0], &cmds[1], sizeof(cmds) - sizeof(cmds[0]));
    rec = &cmds[CMD_RING - 1U];
  }
  memset(rec, 0, sizeof(*rec));
  rec->id = next_cmd++;
  if (next_cmd == 0U)
    next_cmd = 1U;
  rec->state = PSU_CMD_QUEUED;
  rec->source = source;
  rec->mv = mv;
  rec->ma = ma;
  rec->track_readback = track ? 1U : 0U;
  snprintf(rec->name, sizeof(rec->name), "%s", name ? name : "CMD");
  snap.last_cmd_id = rec->id;
  snap.last_cmd_state = rec->state;
  return rec->id;
}

static PsuCmdRec *find_cmd(uint16_t id)
{
  uint8_t i;
  for (i = 0; i < cmd_count; ++i)
    if (cmds[i].id == id)
      return &cmds[i];
  return 0;
}

static void mark_cmd(uint16_t id, uint8_t state, uint32_t now_ms)
{
  PsuCmdRec *rec = find_cmd(id);
  if (rec == 0 || id == 0U)
    return;
  if (rec->state == PSU_CMD_ACK || rec->state == PSU_CMD_REJECTED)
    return;
  rec->state = state;
  if (state == PSU_CMD_SENT)
    rec->sent_ms = now_ms;
  snap.last_cmd_id = rec->id;
  snap.last_cmd_state = rec->state;
}

static void seq_limits(uint32_t mv, uint32_t ma, void *user)
{
  (void)user;
  (void)psu_app_set_limits(mv, ma, PSU_SRC_SEQ);
}

static void seq_output(int on, void *user)
{
  (void)user;
  (void)psu_app_set_output(on, PSU_SRC_SEQ);
}

static int seq_applied(uint32_t *mv, uint32_t *ma, void *user)
{
  (void)user;
  if (!snap.applied_valid)
    return 0;
  if (mv) *mv = snap.applied_mv;
  if (ma) *ma = snap.applied_ma;
  return 1;
}

static int seq_permit(void *user)
{
  (void)user;
  return snap.fault_latched ? 0 : 1;
}

static void chg_limits(uint32_t mv, uint32_t ma, void *user)
{
  (void)user;
  (void)psu_app_set_limits(mv, ma, PSU_SRC_CHARGER);
}

static void chg_output(int on, void *user)
{
  (void)user;
  (void)psu_app_set_output(on, PSU_SRC_CHARGER);
}

static void refresh_snap_from_g0(void)
{
  snap.g0_connected = g0.connected;
  snap.g0_stale = g0.stale;
  snap.current_valid = g0.current_valid;
  snap.vin_mv = g0.connected ? g0.vin_mv : 0U;
  snap.vout_mv = g0.connected ? g0.vout_mv : 0U;
  snap.signed_current_ua = g0.iout_ua;
  snap.display_current_ua = psu_display_current_ua(g0.iout_ua, g0.connected && g0.current_valid);
  snap.power_mw = (g0.connected && g0.current_valid) ? psu_output_power_mw(g0.vout_mv, g0.iout_ua) : 0U;
  snap.mos_centi = g0.connected ? g0.temp_centi_c[0] : 0;
  snap.pcb_centi = g0.connected ? g0.temp_centi_c[3] : 0;
  snap.mode_cc = (g0.connected && g0.mode == 2U) ? 1U : 0U;
  snap.regulation_mode = g0.connected ? g0.mode : 0U;
  snap.temperature_valid = g0.connected && !snap.g4_uart_configured;
  snap.output_confirmed = (g0.connected && (g0.status_flags & 1U)) ? 1U : 0U;
  snap.g0_crc_errors = g0.crc_errors;
  if (g0.connected && g0.current_valid)
  {
    snap.applied_mv = g0.applied_voltage_mv;
    snap.applied_ma = g0.applied_current_ma;
    snap.applied_valid = 1U;
  }
  else if (!g0.connected)
  {
    snap.applied_valid = 0U;
    snap.output_confirmed = 0U;
  }
}

void psu_app_init(void)
{
  memset(&snap, 0, sizeof(snap));
  memset(&g0, 0, sizeof(g0));
  memset(&hooks, 0, sizeof(hooks));
  memset(cmds, 0, sizeof(cmds));
  cmd_count = 0U;
  host_serial = host_tx_count = 0U;
  clear_ack_pending = 0U;
  clear_ack_serial = startup_ms = stop_ms = app_now = 0U;
  energy_seen=0;energy_ms=0;
  g0_seen = 0U;
  g4_init(&g4);
  psu_seq_init(&sequencer);
  psu_chg_init(&charger);
  sequencer.io.limits = seq_limits;
  sequencer.io.output = seq_output;
  sequencer.io.applied = seq_applied;
  sequencer.io.permit_ok = seq_permit;
  charger.io.limits = chg_limits;
  charger.io.output = chg_output;
  fill_defaults();
  apply_blob_to_runtime();
  snap.requested_mv = 12000U;
  snap.requested_ma = 2000U;
  snap.output_requested = 0U;
  snap.cold_output_off = 1U;
  snap.g4_uart_configured = 0U;
  snap.ethernet_configured = 0U;
  snap.net_state = PSU_NET_DOWN;
  snap.g4_link = G4_LINK_OFFLINE;
  snap.bms_valid = 0U;
  snprintf(snap.bms_fault, sizeof(snap.bms_fault), "OFFLINE");
  snprintf(snap.fault, sizeof(snap.fault), "NONE");
  generation = 0U;
  app_ready = 1U;
  touch();
  publish_end();
}

void psu_app_ensure(void)
{
  if (!app_ready)
    psu_app_init();
}

void psu_app_set_hooks(const PsuHalHooks *next)
{
  if (next)
    hooks = *next;
  else
    memset(&hooks, 0, sizeof(hooks));
}

void psu_app_set_g4_uart(int configured) { snap.g4_uart_configured = configured ? 1U : 0U; }

static const char *on_block(uint8_t source)
{
  const G4Telemetry *t = &g4.telemetry;
  if (snap.power_shutdown_requested) return "POWER SHUTDOWN";
  if (snap.fault_latched) return snap.fault;
  if (source == PSU_SRC_WEB && !remote_access) return "REMOTE ACCESS DISABLED";
  if (snap.shutdown_pending) return "WAITING FOR OFF CONFIRMATION";
  if (g4.output_phase == G4_OUTPUT_STOPPING) return "WAITING FOR OFF CONFIRMATION";
  if (source == PSU_SRC_SEQ && sequencer.run != PSU_SEQ_RUN) return "SEQUENCER STOPPED";
  if (source == PSU_SRC_CHARGER && !charger.running) return "CHARGER STOPPED";
  if (!snap.g4_uart_configured) return NULL;
  if (!t->valid) return "WAITING FOR METER";
  if (app_now - t->ms > 50U) return "METER STALE";
  if (!t->g0 || t->g0_age_ms > 500U) return "WAITING FOR FRESH G0";
  if (t->g0_fault & G4_G0_MEAS_LOST) return "WAITING FOR VALID MEASUREMENTS";
  if (g4.event_g0_stale) return "G0 LINK LOSS PENDING";
  if (t->fault || g4.event_fault) return "ACTIVE G4 FAULT";
  if (t->fault_latch || t->ctrl == 12 || g4.event_ctrl_fault) return "G4 CONTROL FAULT";
  if (g4_blocking_g0_faults(t, g4.output_phase) || g4.event_g0_fault) return "ACTIVE G0 FAULT";
  if (snap.cold_output_off) return "WAITING FOR INITIAL OFF";
  if (g4.output_phase == G4_OUTPUT_STARTING) return "START ALREADY PENDING";
  return NULL;
}

static void latch_fault(const char *reason)
{
  clear_ack_pending = 0;
  if (!snap.fault_latched) {
    snap.fault_latched = 1;
    snprintf(snap.fault, sizeof(snap.fault), "%s", reason);
    {
      const G4Telemetry *t=g4.event_meter.valid?&g4.event_meter:&g4.telemetry;
      unsigned phase=g4.event_meter.valid?g4.event_phase:g4.output_phase;
      snprintf(snap.fault_context,sizeof(snap.fault_context),
        "TRIP phase=%u ctrl=%u o/w/p/k=%u%u%u%u G4=%lX G0=%lX age=%lu",
        phase,t->ctrl,t->out,t->want,t->permit,t->kill,
        (unsigned long)t->fault,(unsigned long)t->g0_fault,(unsigned long)t->g0_age_ms);
    }
    /* Cancel queued ON/SET before the priority OFF. Keep the first cause. */
    g4_cancel_pending(&g4);
    (void)psu_app_shutdown();
  }
}

int psu_app_set_limits(uint32_t voltage_mv, uint32_t current_ma, uint8_t source)
{
  if(source == PSU_SRC_WEB && !remote_access)return 0;
  if(snap.power_shutdown_requested)return 0;
  if((source==PSU_SRC_SEQ && sequencer.run!=PSU_SEQ_RUN) ||
     (source==PSU_SRC_CHARGER && !charger.running))return 0;
  uint16_t id;
  uint32_t hw_v = g0.maximum_voltage_mv ? g0.maximum_voltage_mv : PSU_VOLTAGE_MAX_MV;
  uint32_t hw_i = g0.maximum_current_ma ? g0.maximum_current_ma : PSU_CURRENT_MAX_MA;
  if (blob.ovp_mv && blob.ovp_mv < hw_v)
    hw_v = blob.ovp_mv;
  if (blob.ocp_ma && blob.ocp_ma < hw_i)
    hw_i = blob.ocp_ma;
  voltage_mv = psu_clamp_voltage_to_hw(voltage_mv, hw_v);
  current_ma = psu_clamp_current_to_hw(current_ma, hw_i);
  if (snap.fault_latched && source != PSU_SRC_BOOT)
  {
    id = push_cmd("LIMITS", source, voltage_mv, current_ma, 0);
    mark_cmd(id, PSU_CMD_REJECTED, 0U);
    touch();
    publish_end();
    return 0;
  }
  snap.requested_mv = voltage_mv;
  snap.requested_ma = current_ma;
  id = push_cmd("LIMITS", source, voltage_mv, current_ma, 1);
  if(!g4_set_limits(&g4, voltage_mv, current_ma, id)){mark_cmd(id,PSU_CMD_REJECTED,app_now);return 0;}
  if (hooks.ldo_limits)
    hooks.ldo_limits(voltage_mv, current_ma);
  touch();
  publish_end();
  return 1;
}

int psu_app_set_output(int enabled, uint8_t source)
{
  uint16_t id;
  const char *reason = enabled ? on_block(source) : NULL;
  if (!enabled && source == PSU_SRC_WEB && !remote_access) return 0;
  if (reason) {
    snprintf(snap.on_block_reason, sizeof(snap.on_block_reason), "%s", reason);
    snprintf(snap.last_on_reject, sizeof(snap.last_on_reject), "%s", reason);
    id = push_cmd("ON", source, snap.requested_mv, snap.requested_ma, 0);
    mark_cmd(id, PSU_CMD_REJECTED, app_now);
    touch(); publish_end(); return 0;
  }
  snap.output_requested = enabled ? 1U : 0U;
  if(enabled){snap.shutdown_pending=0;snap.shutdown_confirmed=0;}
  {uint8_t k;for(k=0;k<cmd_count;k++)if((!strcmp(cmds[k].name,"ON")||!strcmp(cmds[k].name,"OFF"))&&cmds[k].state<=PSU_CMD_SENT)cmds[k].state=PSU_CMD_REJECTED;}
  id = push_cmd(enabled ? "ON" : "OFF", source, snap.requested_mv, snap.requested_ma, 0);
  if(!(enabled?g4_on(&g4,id):g4_off(&g4,id))){
    mark_cmd(id,PSU_CMD_REJECTED,app_now);
    if(enabled){snap.output_requested=0;snprintf(snap.last_on_reject,sizeof(snap.last_on_reject),"ON TRANSPORT BUSY");}
    touch();publish_end();return 0;
  }
  g4.output_phase = enabled ? G4_OUTPUT_STARTING : G4_OUTPUT_STOPPING;
  if(!enabled) {
    const G4Telemetry *t=&g4.telemetry;
    stop_ms=app_now;
    if(t->valid && app_now-t->ms<=50U && !t->out && !t->want && !t->run &&
       !t->stage_en && !t->ps_en && t->ctrl==0)g4.output_phase=G4_OUTPUT_IDLE;
  }
  snap.output_phase = g4.output_phase;
  if(enabled){startup_ms=app_now;snap.last_on_reject[0]=0;}
  else clear_ack_pending=0;
  if (hooks.ldo_output)
    hooks.ldo_output(enabled ? 1U : 0U);
  if (!g0.connected)
    snap.output_confirmed = 0U;
  touch();
  publish_end();
  return 1;
}

int psu_app_shutdown(void)
{
  uint8_t saved_stop=sequencer.stop_action;
  uint8_t i;
  sequencer.stop_action=PSU_STOP_OFF;
  if(sequencer.run==PSU_SEQ_RUN||sequencer.run==PSU_SEQ_PAUSE)
    psu_seq_stop(&sequencer,app_now,0);
  sequencer.stop_action=saved_stop;
  if(charger.running)psu_chg_abort(&charger,"PSU OFF",app_now);
  g4_cancel_pending(&g4);
  for(i=0;i<cmd_count;i++)if(cmds[i].state<=PSU_CMD_SENT)cmds[i].state=PSU_CMD_REJECTED;
  snap.shutdown_pending=1;snap.shutdown_confirmed=0;
  return psu_app_set_output(0,PSU_SRC_LCD);
}

int psu_app_usb_role(const char *role, uint8_t source)
{
  if(source==PSU_SRC_WEB && !remote_access)return 0;
  uint16_t id;
  (void)source;
  id = push_cmd(role, source, 0U, 0U, 0);
  if (!g4_usb_role(&g4, role, id)){mark_cmd(id,PSU_CMD_REJECTED,app_now);return 0;}
  if (!snap.g4_uart_configured && strcmp(role, "SINK") == 0)
    snap.usb_role = 1U;
  else if (!snap.g4_uart_configured && strcmp(role, "SOURCE") == 0)
    snap.usb_role = 2U;
  else if (!snap.g4_uart_configured)
    snap.usb_role = 0U;
  mark_cmd(id, PSU_CMD_QUEUED, 0U);
  touch();
  publish_end();
  return 1;
}

int psu_app_service(int enabled)
{
  g4.service_mode = enabled ? 1U : 0U;
  snap.service_mode = g4.service_mode;
  touch();
  publish_end();
  return 1;
}

int psu_app_service_cmd(const char *line)
{
  if (!g4_service_line(&g4, line, push_cmd("SVC", PSU_SRC_LCD, 0, 0, 0)))
    return 0;
  touch();
  publish_end();
  return 1;
}

int psu_app_clear_fault(void)
{
  if (!snap.fault_latched)
    return 0;
  if (!snap.g4_uart_configured) {
    snap.fault_latched = 0U;
    snprintf(snap.fault, sizeof(snap.fault), "NONE");
  }
  clear_ack_pending=0;
  if(!g4_simple(&g4, "CLR", push_cmd("CLR", PSU_SRC_LCD, 0, 0, 0)))return 0;
  touch();
  publish_end();
  return 1;
}

int psu_app_bms_cmd(const char *cmd)
{
  if (cmd == 0)
    return 0;
  if (strcmp(cmd, "BMS") != 0 && strcmp(cmd, "BMS FORCE") != 0 && strcmp(cmd, "CLR") != 0)
    return 0;
  if (!g4_simple(&g4, cmd, push_cmd(cmd, PSU_SRC_LCD, 0, 0, 0)))
    return 0;
  if (strcmp(cmd, "BMS OFF") == 0)
  {
    snap.bms_valid = 0U;
    snprintf(snap.bms_fault, sizeof(snap.bms_fault), "BMS OFF");
  }
  touch();
  publish_end();
  return 1;
}

void psu_app_observe_g0(const PsuG0Sample *sample, uint32_t now_ms)
{
  uint8_t was = g0.connected;
  (void)now_ms;
  if (sample == 0)
    return;
  g0 = *sample;
  if (g0.connected && !was)
  {
    snap.output_requested = 0U;
    if (hooks.ldo_output)
      hooks.ldo_output(0U);
    (void)g4_off(&g4, push_cmd("OFF", PSU_SRC_BOOT, 0, 0, 0));
  }
  refresh_snap_from_g0();
  touch();
  publish_end();
}

void psu_app_user_button(int level_high, uint32_t now_ms)
{
  if (level_high != button_level)
  {
    button_level = level_high;
    button_change_ms = now_ms;
    return;
  }
  if ((now_ms - button_change_ms) < 30U)
    return;
  if (button_stable == level_high)
    return;
  button_stable = level_high;
  if (level_high)
    (void)psu_app_set_output(!snap.output_requested, PSU_SRC_BUTTON);
}

uint32_t psu_app_now(void)
{
  return app_now;
}

void psu_app_tick(uint32_t now_ms)
{
  app_now = now_ms;
  uint8_t i;
  g4_process(&g4, now_ms);
  if (snap.g4_uart_configured)
  {
    const G4Telemetry *t = &g4.telemetry;
    const uint32_t age = now_ms - t->ms;
    memset(&g0, 0, sizeof(g0));
    /* g0=0 can mean idle pre-regulation; it is not proof of a dead UART. */
    g0.connected = t->valid && age <= 50U && t->g0 && t->g0_age_ms <= 500U;
    g0.stale = !g0.connected;
    g0.current_valid = g0.connected && !g0.stale && !(t->g0_fault & 256U);
    g0.current_calibrated = g0.current_valid;
    g0.vin_mv = t->g0_vin_mv; g0.vout_mv = t->vout_mv;
    g0.iout_ua = t->iout_ma * 1000;
    g0.status_flags = t->out ? 1U : 0U;
    g0.fault_flags = t->fault | t->g0_fault;
    g0.applied_voltage_mv = t->g0_vset_mv; g0.applied_current_ma = t->g0_iset_ma;
    g0.maximum_voltage_mv = 27000; g0.maximum_current_ma = 5000; g0.mode = t->mode;
    snap.psu_running=t->run;
    if (t->valid && host_serial != t->serial) {
      host_serial = t->serial;
      if(g4.output_phase==G4_OUTPUT_STOPPING && age<=50U && !t->out && !t->want &&
         !t->run && !t->stage_en && !t->ps_en && t->ctrl==0)g4.output_phase=G4_OUTPUT_IDLE;
      if(snap.shutdown_pending) {
        if(age<=50U && !t->run && !t->stage_en && !t->ps_en && !t->out) {
          snap.shutdown_pending=0;snap.shutdown_confirmed=1;
        }
      }
      /* Preserve a user's pending SET until its reply/readback, then follow G4. */
      if (!g4.has_set && !g4.inflight_len[0]) {
        snap.requested_mv = t->set_mv; snap.requested_ma = t->ilim_ma;
      }
      {uint8_t k,pending_output=0;
       for(k=0;k<cmd_count;k++)if((!strcmp(cmds[k].name,"ON")||!strcmp(cmds[k].name,"OFF"))&&cmds[k].state<=PSU_CMD_SENT)pending_output=1;
       if(!pending_output && !t->want)snap.output_requested=0;}
      if (snap.cold_output_off) { snap.cold_output_off = 0; psu_app_set_output(0, PSU_SRC_BOOT); }
    }
    if (g4.event_fault) {
      char why[40];snprintf(why,sizeof(why),"G4 FAULT 0x%lX",(unsigned long)g4.event_fault);latch_fault(why);
    } else if (g4.event_ctrl_fault) latch_fault("G4 CONTROL FAULT");
    else if (g4.event_g0_fault) {
      char why[40];
      if(g4.event_g0_fault&G4_G0_MEAS_LOST)snprintf(why,sizeof(why),"G0 MEAS_LOST");
      else if(g4.event_g0_fault&G4_G0_POWER_KILL)snprintf(why,sizeof(why),"G0 POWER_KILL");
      else snprintf(why,sizeof(why),"G0 FAULT 0x%lX",(unsigned long)g4.event_g0_fault);
      latch_fault(why);
    }
    if(g4.event_g0_stale)latch_fault("H7 G0 STALE");
    g4.event_fault=0;g4.event_g0_fault=0;g4.event_ctrl_fault=0;g4.event_g0_stale=0;
    g4.event_meter.valid=0;
    if(snap.output_requested || t->out || g4.output_phase==G4_OUTPUT_STARTING || g4.output_phase==G4_OUTPUT_RUNNING) {
      if(!t->valid || age>50U)latch_fault("H7 METER STALE");
      else if(!g0.connected && !g4_start_wait(t,g4.output_phase))latch_fault("H7 G0 STALE");
      if(g4.output_phase==G4_OUTPUT_STARTING && now_ms-startup_ms>8000U)latch_fault("H7 START TIMEOUT");
    }
    if(g4.output_phase==G4_OUTPUT_STOPPING && now_ms-stop_ms>800U)latch_fault("H7 OFF TIMEOUT");
    /* CLEAR ACK alone is not evidence that measurements/faults recovered.
       Require a fresh healthy METER received after that ACK, still OFF. */
    if(clear_ack_pending && t->serial!=clear_ack_serial && age<=50U &&
       g0.connected && g0.current_valid && !t->fault && !t->fault_latch && t->ctrl!=12 &&
       !g4_blocking_g0_faults(t,G4_OUTPUT_IDLE) && !t->out && !t->want) {
      clear_ack_pending=0;snap.fault_latched=0;snprintf(snap.fault,sizeof(snap.fault),"NONE");
    }
    snap.bms_valid = g4.battery.valid && now_ms-g4.battery.ms<=1000U && g4.battery.present && g4.battery.configured && g4.battery.sample;
    memset(snap.cell_mv,0,sizeof(snap.cell_mv));
    /* BQ76922 4S uses physical taps 1,2,3,5 (c4=-1), compact for four UI cells. */
    {unsigned k=0,j;for(j=0;j<5&&k<4;j++)if(g4.battery.cell_mv[j]>0)snap.cell_mv[k++]=(uint16_t)g4.battery.cell_mv[j];}
    snap.bms_fet_chg=g4.battery.chg;snap.bms_fet_dsg=g4.battery.dsg;
    if(g4.battery.fault)snprintf(snap.bms_fault,sizeof(snap.bms_fault),"BMS 0x%lX",(unsigned long)g4.battery.fault);
    else snprintf(snap.bms_fault,sizeof(snap.bms_fault),"%s",snap.bms_valid?"":"NO PACK DATA");
    if(g4.charger.valid && now_ms-g4.charger.ms<=1000U)snap.usb_role=g4.charger.pd_role;
    if (host_tx_count != g4.tx_count) {host_tx_count=g4.tx_count;mark_cmd(g4.last_tx_id,PSU_CMD_SENT,now_ms);}
    while (g4_response(&g4,&g4.response_id,&g4.response_state)) {
      PsuCmdRec *r=find_cmd(g4.response_id);
      if(r && g4.response_state==PSU_CMD_ACK && !strcmp(r->name,"CLR")){clear_ack_pending=1;clear_ack_serial=t->serial;}
      if(r && !strcmp(r->name,"ON") && g4.response_state==PSU_CMD_ACK && snap.output_requested)g4.output_phase=G4_OUTPUT_RUNNING;
      if(r && !strcmp(r->name,"ON") && g4.response_state==PSU_CMD_REJECTED && snap.output_requested)latch_fault("H7 ON REJECTED BY G4");
      if(r)mark_cmd(g4.response_id,g4.response_state,now_ms);
      if(r && g4.response_state==PSU_CMD_TIMEOUT && (!strcmp(r->name,"LIMITS") || !strcmp(r->name,"ON"))){
        latch_fault("H7 COMMAND TIMEOUT");
      }
      g4.response_id=0;
    }
  }
  refresh_snap_from_g0();
  if(snap.g4_uart_configured){
    const G4Record *a=&g4.records[G4_RECORD_AUX];int64_t v;unsigned k;
    snap.temperature_valid=a->valid && now_ms-a->ms<=1000 && g4_record_value(a,"valid",&v)&&v;
    for(k=0;k<4;k++){char key[4];snprintf(key,sizeof(key),"t%u",k+1);if(!snap.temperature_valid||!g4_record_value(a,key,&v))g0.temp_centi_c[k]=INT16_MIN;else g0.temp_centi_c[k]=(int16_t)(v*10);}
    snap.temperature_valid=snap.temperature_valid&&g0.temp_centi_c[0]!=INT16_MIN&&g0.temp_centi_c[3]!=INT16_MIN;
    snap.mos_centi=g0.temp_centi_c[0];snap.pcb_centi=g0.temp_centi_c[3];
    snap.bms_balancing=0;if(g4_record_value(&g4.records[1],"balance_mask",&v)&&snap.bms_valid)snap.bms_balancing=(uint8_t)v;
    snap.battery_energy_valid=snap.bms_valid;
    if(snap.bms_valid&&energy_seen&&now_ms-energy_ms<=50){snap.battery_energy_uwms+=(int64_t)g4.battery.pack_mv*g4.battery.current_ma*(now_ms-energy_ms);}
    energy_seen=snap.bms_valid;energy_ms=now_ms;
  }
  for (i = 0; i < cmd_count; ++i)
  {
    if (!snap.g4_uart_configured && cmds[i].state == PSU_CMD_QUEUED)
    {
      cmds[i].state = PSU_CMD_SENT;
      cmds[i].sent_ms = now_ms;
      snap.last_cmd_state = cmds[i].state;
    }
    else if (cmds[i].state == PSU_CMD_SENT && !snap.g4_uart_configured && g0.connected && !g0.stale &&
             ((!strcmp(cmds[i].name,"ON") && snap.output_confirmed) ||
              (!strcmp(cmds[i].name,"OFF") && !snap.output_confirmed)))
      mark_cmd(cmds[i].id, PSU_CMD_ACK, now_ms);
    else if (cmds[i].state == PSU_CMD_SENT && !snap.g4_uart_configured && cmds[i].track_readback &&
             snap.applied_valid &&
             snap.applied_mv == cmds[i].mv && snap.applied_ma == cmds[i].ma)
    {
      cmds[i].state = PSU_CMD_ACK;
      snap.last_cmd_state = PSU_CMD_ACK;
    }
    else if (cmds[i].state == PSU_CMD_SENT &&
             (cmds[i].track_readback || !strcmp(cmds[i].name,"ON") || !strcmp(cmds[i].name,"OFF")) &&
             (now_ms - cmds[i].sent_ms) > (!strcmp(cmds[i].name,"ON")?8000U:G4_CMD_TIMEOUT_MS))
    {
      cmds[i].state = PSU_CMD_TIMEOUT;
      snap.last_cmd_state = PSU_CMD_TIMEOUT;
    }
  }
  psu_seq_tick(&sequencer, now_ms);
  snap.seq_run = sequencer.run;
  snap.seq_count = sequencer.count;
  snap.seq_step = (uint8_t)(sequencer.step_index + 1U);
  snap.seq_mv = sequencer.ramp_mv;
  snap.seq_ma = sequencer.count ? sequencer.steps[sequencer.step_index < sequencer.count ? sequencer.step_index : 0].current_ma : 0U;
  snprintf(snap.seq_status, sizeof(snap.seq_status), "%s", sequencer.status);
  if (sequencer.run == PSU_SEQ_RUN)
  {
    snap.requested_mv = sequencer.ramp_mv ? sequencer.ramp_mv : snap.requested_mv;
    snap.requested_ma = snap.seq_ma;
  }
  {
    PsuChgSense sense;
    memset(&sense, 0, sizeof(sense));
    sense.pack_mv = snap.vout_mv;
    sense.current_ma = (int32_t)(snap.display_current_ua / 1000);
    sense.telemetry_ok = g0.connected && !g0.stale && g0.current_valid && (!charger.profile.temp_sensor || snap.temperature_valid);
    sense.permit = !snap.fault_latched && (!snap.g4_uart_configured || g4.telemetry.permit);
    sense.temp_centi = snap.mos_centi;
    if (charger.running)
      psu_chg_tick(&charger, &sense, now_ms);
  }
  snap.chg_state = (uint8_t)charger.state;
  snprintf(snap.chg_reason, sizeof(snap.chg_reason), "%s", charger.reason);
  snap.g4_link = (uint8_t)g4.link;
  snap.pps_allowed = (uint8_t)g4_pps_allowed(&g4);
  snprintf(snap.g4_last_tx, sizeof(snap.g4_last_tx), "%.47s", g4.last_tx);
  snprintf(snap.g4_last_rx, sizeof(snap.g4_last_rx), "%.47s", g4.last_rx);
  snap.g4_overflow = g4.overflow_count;
  snap.service_mode = g4.service_mode;
  if (g0.connected)
    g0_seen = 1U;
  if (!snap.g4_uart_configured && g0_seen && !g0.connected && snap.output_requested && !snap.fault_latched)
  {
    snap.fault_latched = 1U;
    snprintf(snap.fault, sizeof(snap.fault), "G0 OFFLINE");
    (void)psu_app_set_output(0, PSU_SRC_BOOT);
  }
  if (g0.connected && blob.ovp_mv && g0.vout_mv > blob.ovp_mv && !snap.fault_latched)
  {
    latch_fault("H7 OVP");
  }
  snap.output_phase=g4.output_phase;
  {const char *reason=on_block(PSU_SRC_LCD);snprintf(snap.on_block_reason,sizeof(snap.on_block_reason),"%s",reason?reason:"READY");}
  snap.nack_valid=g4.nack_valid;snap.nack_type=g4.nack_type;snap.nack_seq=g4.nack_seq;
  snap.nack_reason=g4.nack_reason;snap.nack_matched=g4.nack_matched;

}

void psu_snapshot(PsuSnapshot *out)
{
  uint32_t before;
  uint32_t after;
  if (out == 0)
    return;
  unsigned spins;
  for (spins = 0U; spins < 8U; ++spins)
  {
    before = generation;
    if (before & 1U)
      continue;
    __asm__ volatile("" ::: "memory");
    *out = snap;
    __asm__ volatile("" ::: "memory");
    after = generation;
    if (after == before)
      return;
  }
  *out = snap;
}

int psu_preset_apply(uint8_t index)
{
  if (index >= PSU_PRESET_COUNT || index >= blob.preset_count)
    return 0;
  snap.preset_selected = (uint8_t)(index + 1U);
  return psu_app_set_limits(blob.presets[index].voltage_mv, blob.presets[index].current_ma, PSU_SRC_LCD);
}

int psu_preset_set(uint8_t index, uint32_t mv, uint32_t ma)
{
  if (index >= PSU_PRESET_COUNT || mv > 27000U || ma > 5000U) return 0;
  blob.presets[index].voltage_mv = mv;
  blob.presets[index].current_ma = ma;
  blob.presets[index].output_action = 0;
  snprintf(blob.presets[index].name, sizeof(blob.presets[index].name), "PRESET %u", index + 1U);
  if (blob.preset_count < 3U) blob.preset_count = 3U;
  touch(); publish_end();
  return 1;
}

int psu_preset_save_current(uint8_t index, const char *name)
{
  if (index >= PSU_PRESET_COUNT)
    return 0;
  if (index >= PSU_PRESET_COUNT || index >= blob.preset_count)
    blob.preset_count = (uint8_t)(index + 1U);
  blob.presets[index].voltage_mv = snap.requested_mv;
  blob.presets[index].current_ma = snap.requested_ma;
  blob.presets[index].output_action = 0U;
  snprintf(blob.presets[index].name, sizeof(blob.presets[index].name), "%s", name ? name : "PRESET");
  touch();
  publish_end();
  return 1;
}

int psu_preset_duplicate(uint8_t from, uint8_t to)
{
  if (from >= blob.preset_count || to >= PSU_PRESET_COUNT)
    return 0;
  blob.presets[to] = blob.presets[from];
  blob.presets[to].output_action = 0U;
  if (to >= blob.preset_count)
    blob.preset_count = (uint8_t)(to + 1U);
  return 1;
}

void psu_preset_reset_defaults(void)
{
  memset(blob.presets, 0, sizeof(blob.presets));
  blob.preset_count = 3U;
  const uint32_t mv[3] = {5000U, 12000U, 20000U};
  const uint32_t ma[3] = {1000U, 2000U, 3000U};
  const char *names[3] = {"5V 1A", "12V 2A", "20V 3A"};
  uint8_t i;
  for (i = 0; i < 3U; ++i) {
    blob.presets[i].voltage_mv = mv[i];
    blob.presets[i].current_ma = ma[i];
    snprintf(blob.presets[i].name, sizeof(blob.presets[i].name), "%s", names[i]);
  }
  snap.preset_selected = 0U;
  touch();
  publish_end();
}

const PsuPreset *psu_preset_get(uint8_t index)
{
  if (index >= PSU_PRESET_COUNT || index >= blob.preset_count)
    return 0;
  return &blob.presets[index];
}

PsuSequencer *psu_sequencer(void) { return &sequencer; }
PsuCharger *psu_charger(void) { return &charger; }
G4Port *psu_g4(void) { return &g4; }

void psu_net_down(void)
{
  snap.net_state = PSU_NET_DOWN;
  memset(snap.ip, 0, sizeof(snap.ip));
  memset(snap.mask, 0, sizeof(snap.mask));
  memset(snap.gateway, 0, sizeof(snap.gateway));
  touch();
  publish_end();
}

void psu_net_up(void)
{
  snap.net_state = PSU_NET_DHCP;
  memset(snap.ip, 0, sizeof(snap.ip));
  touch();
  publish_end();
}

int psu_net_bound(const uint8_t ip[4], const uint8_t mask[4], const uint8_t gw[4])
{
  if (ip == 0 || (ip[0] | ip[1] | ip[2] | ip[3]) == 0)
    return 0;
  if (snap.net_state != PSU_NET_DHCP && snap.net_state != PSU_NET_BOUND)
    return 0;
  memcpy(snap.ip, ip, 4);
  if (mask) memcpy(snap.mask, mask, 4);
  if (gw) memcpy(snap.gateway, gw, 4);
  snap.net_state = PSU_NET_BOUND;
  touch();
  publish_end();
  return 1;
}

void psu_net_dhcp_timeout(void)
{
  snap.net_state = PSU_NET_TIMEOUT;
  memset(snap.ip, 0, sizeof(snap.ip));
  touch();
  publish_end();
}

void psu_net_set_mac(const uint8_t mac[6])
{
  if (mac)
    memcpy(snap.mac, mac, 6);
}

void psu_config_bind_ram(uint8_t *slot_a, uint8_t *slot_b, size_t bytes)
{
  store_a = slot_a;
  store_b = slot_b;
  store_n = bytes;
  psu_store_init(&store, slot_a, slot_b, bytes);
}

static void blob_from_runtime(void)
{
  uint8_t i;
  blob.output_on = 0U;
  blob.step_count = sequencer.count;
  blob.seq_mode = sequencer.mode;
  blob.seq_loops = sequencer.loops_requested;
  for (i = 0; i < sequencer.count; ++i)
    blob.steps[i] = sequencer.steps[i];
}

int psu_config_save(void)
{
  if (store_a == 0)
    return 0;
  blob_from_runtime();
  blob.output_on = 0U;
  return psu_store_save(&store, &blob, (uint16_t)sizeof(blob));
}

int psu_config_load(void)
{
  PsuBlob loaded;
  if (store_a == 0)
    return 0;
  memset(&loaded, 0, sizeof(loaded));
  if (!psu_store_load(&store, &loaded, (uint16_t)sizeof(loaded)))
  {
    fill_defaults();
    apply_blob_to_runtime();
    return 0;
  }
  loaded.output_on = 0U;
  blob = loaded;
  apply_blob_to_runtime();
  snap.output_requested = 0U;
  snap.output_confirmed = 0U;
  touch();
  publish_end();
  return 1;
}

static int lease_ok(const char *client, uint32_t now_hint)
{
  (void)now_hint;
  if (client == 0 || client[0] == '\0')
    return 0;
  if (lease_client[0] == '\0' || strcmp(lease_client, client) == 0)
  {
    snprintf(lease_client, sizeof(lease_client), "%s", client);
    return 1;
  }
  return 0;
}

int psu_web_write_state(char *dst, size_t n)
{
  PsuSnapshot s;
  if (dst == 0 || n < 32U)
    return 0;
  psu_snapshot(&s);
  return snprintf(dst, n,
                  "{\"rev\":%u,\"requested_mv\":%u,\"requested_ma\":%u,"
                  "\"applied_mv\":%u,\"applied_ma\":%u,\"applied_valid\":%u,"
                  "\"output_requested\":%u,\"output_confirmed\":%u,"
                  "\"vout_mv\":%u,\"vin_mv\":%u,\"iout_ua\":%d,\"power_mw\":%u,"
                  "\"g0_online\":%u,\"g4_link\":%u,\"pps_allowed\":%u,"
                  "\"seq_status\":\"%s\",\"seq_mv\":%u,\"seq_ma\":%u,"
                  "\"fault\":%u,\"net\":%u,\"ip\":\"%u.%u.%u.%u\","
                  "\"cmd\":%u,\"cmd_state\":%u}",
                  (unsigned)s.revision, (unsigned)s.requested_mv, (unsigned)s.requested_ma,
                  (unsigned)s.applied_mv, (unsigned)s.applied_ma, s.applied_valid,
                  s.output_requested, s.output_confirmed,
                  (unsigned)s.vout_mv, (unsigned)s.vin_mv, (int)s.signed_current_ua, (unsigned)s.power_mw,
                  s.g0_connected, s.g4_link, s.pps_allowed,
                  s.seq_status, (unsigned)s.seq_mv, (unsigned)s.seq_ma,
                  s.fault_latched, s.net_state,
                  s.ip[0], s.ip[1], s.ip[2], s.ip[3],
                  s.last_cmd_id, s.last_cmd_state);
}

static int json_u32(const char *body, const char *key, uint32_t *out)
{
  char pat[32];
  const char *p;
  snprintf(pat, sizeof(pat), "\"%s\":", key);
  p = strstr(body, pat);
  if (p == 0)
    return 0;
  p += strlen(pat);
  *out = 0U;
  while (*p >= '0' && *p <= '9')
  {
    *out = (*out * 10U) + (uint32_t)(*p - '0');
    ++p;
  }
  return 1;
}

int psu_web_request(const char *method, const char *path, const char *body,
                    const char *client, char *dst, size_t n)
{
  if(!remote_access) {if(dst&&n)dst[0]=0;return 0;}

  if (dst == 0 || n < 16U || method == 0 || path == 0)
    return 0;
  if (strcmp(method, "GET") == 0 && strcmp(path, "/api/state") == 0)
    return psu_web_write_state(dst, n);
  if (strcmp(method, "GET") == 0 && strcmp(path, "/") == 0)
  {
    const char *page = psu_web_page();
    snprintf(dst, n, "%s", page);
    return (int)strlen(dst);
  }
  if (strcmp(method, "POST") == 0 && strcmp(path, "/api/command") == 0)
  {
    uint32_t mv = 0, ma = 0, en = 0;
    if (!lease_ok(client, 0U))
    {
      snprintf(dst, n, "{\"result\":\"rejected\",\"reason\":\"lease\"}");
      return (int)strlen(dst);
    }
    if (body && json_u32(body, "voltage_mv", &mv) && json_u32(body, "current_ma", &ma))
    {
      int ok = psu_app_set_limits(mv, ma, PSU_SRC_WEB);
      snprintf(dst, n, "{\"result\":\"%s\",\"requested_mv\":%u,\"requested_ma\":%u}",
               ok ? "queued" : "rejected", (unsigned)snap.requested_mv, (unsigned)snap.requested_ma);
      return (int)strlen(dst);
    }
    if (body && json_u32(body, "output", &en))
    {
      int ok = psu_app_set_output(en ? 1 : 0, PSU_SRC_WEB);
      snprintf(dst, n, "{\"result\":\"%s\",\"output_requested\":%u}",
               ok ? "queued" : "rejected", snap.output_requested);
      return (int)strlen(dst);
    }
    snprintf(dst, n, "{\"result\":\"rejected\"}");
    return (int)strlen(dst);
  }
  snprintf(dst, n, "{\"result\":\"not_found\"}");
  return (int)strlen(dst);
}

size_t psu_http_next(const char *body, size_t len, size_t *offset, char *dst, size_t window)
{
  size_t left;
  size_t n;
  if (body == 0 || offset == 0 || dst == 0 || window == 0U || *offset >= len)
    return 0U;
  left = len - *offset;
  n = left < window ? left : window;
  memcpy(dst, body + *offset, n);
  *offset += n;
  return n;
}

const char *psu_web_page(void)
{
  if (web_page_storage[0] == '\0')
  {
    snprintf(web_page_storage, sizeof(web_page_storage),
             "<!doctype html><html><head><meta charset=\"utf-8\">"
             "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
             "<title>Digital PD PSU</title>"
             "<style>body{margin:0;background:#07140f;color:#e7f6ee;font:16px/1.4 sans-serif}"
             ".wrap{max-width:960px;margin:0 auto;padding:16px}"
             ".card{background:#10281c;border:1px solid #1f6b45;border-radius:12px;padding:16px;margin:0 0 12px}"
             "h1{font-size:22px;margin:0 0 8px}button{min-height:44px;margin:4px;padding:8px 14px;"
             "background:#123;color:#9ff;border:1px solid #2ac7d9;border-radius:8px}"
             ".ok{color:#3ddc84}.bad{color:#ff5c5c}</style></head><body><div class=\"wrap\">"
             "<h1>Digital PD PSU</h1><p>Lab console. Values come from the instrument model.</p>"
             "<div class=\"card\" id=\"live\">Waiting for /api/state</div>"
             "<div class=\"card\"><button onclick=\"cmd({output:0})\">Output off</button>"
             "<button onclick=\"cmd({voltage_mv:5000,current_ma:1000})\">5 V / 1 A</button>"
             "<button onclick=\"cmd({voltage_mv:12000,current_ma:2000})\">12 V / 2 A</button></div>"
             "</div><script>"
             "async function poll(){const r=await fetch('/api/state');document.getElementById('live').textContent=await r.text();}"
             "async function cmd(b){await fetch('/api/command',{method:'POST',headers:{'Content-Type':'application/json','X-Client':'local'},body:JSON.stringify(b)});poll();}"
             "setInterval(poll,500);poll();"
             "</script></body></html>");
  }
  return web_page_storage;
}


int psu_remote_enabled(void){return remote_access!=0;}
void psu_remote_enable(int enabled){remote_access=enabled?1:0;lease_client[0]=0;if(!enabled)psu_net_down();}

int psu_app_power_shutdown(void){psu_app_shutdown();snap.power_shutdown_requested=1;return g4_power_shutdown(&g4,push_cmd("POWER OFF",PSU_SRC_LCD,0,0,0));}
