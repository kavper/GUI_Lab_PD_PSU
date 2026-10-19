#include "g0_frame.h"
#include "g4_ascii.h"
#include "psu_app.h"
#include "psu_bms.h"
#include "psu_charger.h"
#include "psu_edit.h"
#include "psu_energy.h"
#include "psu_format.h"
#include "psu_limits.h"
#include "psu_pd.h"
#include "psu_seq.h"
#include "psu_store.h"
#include "psu_swipe.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static int failures;

static void expect(int cond, const char *msg)
{
  if (!cond)
  {
    fprintf(stderr, "FAIL %s\n", msg);
    ++failures;
  }
}

static void put_u32(uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void put_u16(uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static void test_g0(void)
{
  uint8_t payload[68];
  G0RawTelemetry raw;
  G0FrameParser parser;
  uint8_t frame[80];
  uint8_t out[80];
  uint8_t type = 0, seq = 0, plen = 0;
  uint16_t crc;
  int i;
  int32_t current = -123456;
  memset(payload, 0, sizeof(payload));
  payload[0] = 1;
  payload[1] = 1;
  put_u16(payload + 2, 0x0010);
  put_u32(payload + 16, 12040);
  put_u32(payload + 20, (uint32_t)current);
  put_u32(payload + 24, 0x00ABCDEF);
  expect(g0_decode_telemetry68(payload, &raw) == 0, "decode");
  expect(raw.status_flags == 0x0010, "status offset 2");
  expect(raw.iout_ua == current, "signed current offset 20");
  expect(raw.iout_adc_raw == 0x00ABCDEF, "adc raw offset 24");
  expect(raw.vout_mv == 12040, "vout");
  expect(psu_output_power_mw(12040, current) == 0U, "negative current power");
  expect(psu_display_current_ua(current, 1) == 0, "negative display current");
  expect(psu_display_current_ua(1500, 0) == 0, "invalid current");
  expect(psu_output_power_mw(27000, 5000000) == 135000U, "power 27V*5A");
  expect(psu_output_power_mw(4000000000U, 2000000000) > 1000U, "power no wrap");

  memset(frame, 0, sizeof(frame));
  frame[0] = 0xA5;
  frame[1] = 0x5A;
  frame[2] = 70;
  frame[3] = 0x80;
  frame[4] = 3;
  memcpy(frame + 5, payload, 68);
  crc = g0_crc16_ccitt(frame + 2, 71);
  frame[73] = (uint8_t)crc;
  frame[74] = (uint8_t)(crc >> 8);
  g0_parser_init(&parser);
  for (i = 0; i < 8; ++i)
    expect(g0_parser_push(&parser, 0x11, &type, &seq, out, &plen) == 0, "garbage");
  for (i = 0; i < 75; ++i)
  {
    int got = g0_parser_push(&parser, frame[i], &type, &seq, out, &plen);
    if (i < 74)
      expect(got == 0, "partial");
    else
    {
      expect(got == 1, "frame");
      expect(type == 0x80 && seq == 3 && plen == 68, "header");
      expect(g0_decode_telemetry68(out, &raw) == 0, "payload from frame");
      expect(raw.iout_ua == current, "frame current");
    }
  }
  frame[74] ^= 0xFF;
  g0_parser_init(&parser);
  for (i = 0; i < 75; ++i)
    g0_parser_push(&parser, frame[i], &type, &seq, out, &plen);
  expect(parser.crc_errors == 1U, "crc error");
}

static void test_format_edit(void)
{
  char buf[32];
  PsuEditor ed;
  uint32_t milli = 0;
  psu_format_voltage(buf, sizeof(buf), 5040);
  expect(strcmp(buf, "5.040 V") == 0, "voltage below 10");
  psu_format_voltage(buf, sizeof(buf), 12040);
  expect(strcmp(buf, "12.04 V") == 0, "voltage from 10");
  psu_editor_load_milli(&ed, 12000, 2);
  psu_editor_key(&ed, '9');
  expect(strcmp(ed.text, "9") == 0, "first key replaces");
  psu_editor_key(&ed, '.');
  psu_editor_key(&ed, '5');
  expect(psu_editor_parse_milli(&ed, &milli) == 1, "parse");
  expect(milli == 9500U, "9.5 V");
  expect(psu_clamp_voltage_mv(28000) == 27000U, "voltage clamp");
  expect(psu_clamp_current_ma(9000) == 5000U, "current clamp");
}

static void test_g4(void)
{
  G4Port port;
  char line[96];
  const char *chunk = "STA";
  const char *rest = "TUS\r\nON\n";
  int n = 0;
  g4_init(&port);
  g4_link_up(&port);
  expect(g4_set_mv(&port, 12000, 1) == 1, "set");
  expect(g4_set_mv(&port, 12500, 2) == 1, "coalesce set");
  expect(g4_ilim_ma(&port, 1500, 3) == 1, "ilim");
  expect(g4_pop_tx(&port, line, sizeof(line), 0) == 1, "tel");
  expect(strcmp(line, "TEL 500\n") == 0, "session tel");
  expect(g4_pop_tx(&port, line, sizeof(line), 0) == 1, "status");
  expect(strcmp(line, "STATUS\n") == 0, "session status");
  expect(g4_pop_tx(&port, line, sizeof(line), 1000) == 1, "ilim tx");
  expect(strcmp(line, "ILIM 1.500\n") == 0, "ilim text");
  expect(g4_pop_tx(&port, line, sizeof(line), 1000) == 1, "set tx");
  expect(strcmp(line, "SET 12.500\n") == 0, "latest set");
  expect(g4_pop_tx(&port, line, sizeof(line), 1000) == 0, "rate limit");
  expect(g4_off(&port, 9) == 1, "off queued");
  {
    int i;
    for (i = 0; i < 20; ++i)
      g4_enqueue(&port, "STATUS", 0, 4);
  }
  expect(g4_off(&port, 10) == 1, "off despite full queue");
  expect(g4_pop_tx(&port, line, sizeof(line), 1000) == 1, "safety first");
  expect(strcmp(line, "OFF\n") == 0, "off text");
  expect(g4_pps(&port, 9000, 2500, 1) == 0, "pps locked");
  g4_set_pps_gates(&port, 1, 1, 1);
  expect(g4_pps_allowed(&port) == 1, "pps open");
  expect(g4_pps(&port, 9000, 2500, 8) == 1, "pps");
  {
    int found = 0;
    int guard = 0;
    while (guard++ < 16 && g4_pop_tx(&port, line, sizeof(line), 5000))
    {
      if (strcmp(line, "PPS 9.00 2.50\n") == 0)
        found = 1;
    }
    expect(found == 1, "pps format");
  }
  g4_rx_bytes(&port, (const uint8_t *)chunk, 3, 10);
  g4_rx_bytes(&port, (const uint8_t *)rest, strlen(rest), 11);
  expect(port.rx_lines == 2U, "split lines");
  expect(port.link == G4_LINK_ONLINE, "rx online");
  n = 0;
  while (g4_pop_tx(&port, line, sizeof(line), 5000))
    ++n;
  expect(n < 30, "queue bounded");
}

static uint32_t seq_mv, seq_ma;
static int seq_on;
static int seq_have;
static void on_limits(uint32_t mv, uint32_t ma, void *user)
{
  (void)user;
  seq_mv = mv;
  seq_ma = ma;
  seq_have = 1;
}
static void on_output(int on, void *user)
{
  (void)user;
  seq_on = on;
}
static int on_applied(uint32_t *mv, uint32_t *ma, void *user)
{
  (void)user;
  if (!seq_have)
    return 0;
  *mv = seq_mv;
  *ma = seq_ma;
  return 1;
}

static void test_seq(void)
{
  PsuSequencer seq;
  PsuSeqStep step;
  int i;
  psu_seq_init(&seq);
  seq.io.limits = on_limits;
  seq.io.output = on_output;
  seq.io.applied = on_applied;
  seq.io.permit_ok = 0;
  expect(seq.count == 1U, "default one step");
  for (i = 0; i < 11; ++i)
    expect(psu_seq_add(&seq) == 1, "add");
  expect(psu_seq_add(&seq) == 0, "cap 12");
  expect(seq.count == 12U, "twelve");
  seq.selected = 3;
  expect(psu_seq_remove_selected(&seq) == 1, "remove");
  expect(seq.count == 11U, "after remove");
  memset(&step, 0, sizeof(step));
  step.voltage_mv = 5000;
  step.current_ma = 1000;
  step.time_ms = 100;
  step.enabled = 0;
  step.output_action = PSU_STEP_ON;
  expect(psu_seq_set_step(&seq, 0, &step) == 1, "skip step");
  step.enabled = 1;
  step.output_action = PSU_STEP_KEEP;
  step.slew_mv_per_s = 0;
  expect(psu_seq_set_step(&seq, 1, &step) == 1, "step 2");
  seq_have = 0;
  seq_on = 0;
  expect(psu_seq_start(&seq, 0) == 1, "start");
  psu_seq_tick(&seq, 10);
  expect(seq_on == 0, "start does not force output");
  expect(seq_mv == 5000U && seq_ma == 1000U, "emits limits");
  psu_seq_tick(&seq, 200);
  expect(seq.run == PSU_SEQ_RUN || seq.run == PSU_SEQ_DONE, "advances");
  psu_seq_stop(&seq, 300, 0);
  expect(seq.config_locked == 0U, "unlocked after stop");
  /* timeout */
  psu_seq_init(&seq);
  seq.io.limits = on_limits;
  seq.io.applied = 0;
  seq.steps[0].time_ms = 5000;
  seq.steps[0].enabled = 1;
  psu_seq_start(&seq, 0);
  psu_seq_tick(&seq, 10);
  psu_seq_tick(&seq, 800);
  expect(seq.run == PSU_SEQ_ABORTED, "timeout abort");
  expect(strcmp(seq.status, "ABORTED / CONTROLLER TIMEOUT") == 0, "timeout text");
}

static void type_text(PsuSeqEdit *ed, PsuSequencer *seq, const char *s)
{
  while (*s)
  {
    expect(psu_seq_edit_key(ed, seq, *s) == 1, "key accepted");
    ++s;
  }
}

static void test_seq_edit(void)
{
  PsuSequencer seq;
  PsuSeqEdit ed;
  char cell[16];
  uint32_t kept_v;
  uint8_t kept_out;
  psu_seq_init(&seq);
  psu_seq_edit_init(&ed);
  kept_v = seq.steps[0].voltage_mv;
  kept_out = seq.steps[0].output_action;
  expect(psu_seq_edit_select(&ed, &seq, PSU_SEQ_FIELD_V) == 1, "select voltage");
  type_text(&ed, &seq, "30");
  psu_seq_edit_cell(cell, sizeof(cell), &ed, &seq.steps[0]);
  expect(strcmp(cell, "30") == 0, "live voltage string");
  expect(psu_seq_edit_apply(&ed, &seq) == 0, "reject 30 V");
  expect(seq.steps[0].voltage_mv == kept_v, "voltage unchanged after reject");
  psu_seq_edit_cell(cell, sizeof(cell), &ed, &seq.steps[0]);
  expect(strcmp(cell, "RANGE") == 0, "cell shows range");
  type_text(&ed, &seq, "12.5");
  expect(psu_seq_edit_apply(&ed, &seq) == 1, "apply 12.5 V");
  expect(seq.steps[0].voltage_mv == 12500U, "voltage stored");
  expect(seq.steps[0].output_action == kept_out, "output stays KEEP");
  psu_seq_edit_cell(cell, sizeof(cell), &ed, &seq.steps[0]);
  expect(strcmp(cell, "12.50 V") == 0, "cell shows applied voltage");

  expect(psu_seq_edit_next(&ed, &seq) == 1, "next is current");
  expect(ed.field == PSU_SEQ_FIELD_I, "field current");
  type_text(&ed, &seq, "6");
  expect(psu_seq_edit_apply(&ed, &seq) == 0, "reject 6 A");
  expect(seq.steps[0].current_ma == 1000U, "current unchanged");
  type_text(&ed, &seq, "1.5");
  expect(psu_seq_edit_apply(&ed, &seq) == 1, "apply 1.5 A");
  expect(seq.steps[0].current_ma == 1500U, "current stored");

  expect(psu_seq_edit_next(&ed, &seq) == 1, "next is time");
  expect(ed.field == PSU_SEQ_FIELD_T, "field time");
  type_text(&ed, &seq, "0.05");
  expect(psu_seq_edit_apply(&ed, &seq) == 0, "reject 0.05 s");
  expect(seq.steps[0].time_ms == 1000U, "time unchanged");
  type_text(&ed, &seq, "0.1");
  expect(psu_seq_edit_apply(&ed, &seq) == 1, "apply 0.1 s");
  expect(seq.steps[0].time_ms == 100U, "time stored");
  type_text(&ed, &seq, "3600");
  expect(psu_seq_edit_apply(&ed, &seq) == 1, "apply 3600 s");
  expect(seq.steps[0].time_ms == 3600000U, "max time stored");
  type_text(&ed, &seq, "3601");
  expect(psu_seq_edit_apply(&ed, &seq) == 0, "reject 3601 s");
  expect(seq.steps[0].time_ms == 3600000U, "time stays at max");

  expect(psu_seq_edit_next(&ed, &seq) == 1, "next is slew");
  expect(ed.field == PSU_SEQ_FIELD_S, "field slew");
  type_text(&ed, &seq, "0");
  expect(psu_seq_edit_apply(&ed, &seq) == 1, "apply immediate slew");
  expect(seq.steps[0].slew_mv_per_s == 0U, "slew zero");
  type_text(&ed, &seq, "0.001");
  expect(psu_seq_edit_apply(&ed, &seq) == 1, "apply 0.001 V/s");
  expect(seq.steps[0].slew_mv_per_s == 1U, "slew 1 mV/s");
  type_text(&ed, &seq, "100");
  expect(psu_seq_edit_apply(&ed, &seq) == 1, "apply 100 V/s");
  expect(seq.steps[0].slew_mv_per_s == 100000U, "slew 100 V/s");
  type_text(&ed, &seq, "101");
  expect(psu_seq_edit_apply(&ed, &seq) == 0, "reject 101 V/s");
  expect(seq.steps[0].slew_mv_per_s == 100000U, "slew unchanged");
  type_text(&ed, &seq, "0.0001");
  expect(psu_seq_edit_apply(&ed, &seq) == 0, "reject extra slew digits");
  expect(seq.steps[0].slew_mv_per_s == 100000U, "slew still unchanged");

  expect(psu_seq_edit_next(&ed, &seq) == 1, "wrap to voltage");
  expect(ed.field == PSU_SEQ_FIELD_V, "wrapped field");
  psu_seq_start(&seq, 0);
  expect(psu_seq_edit_locked(&seq) == 1, "locked while running");
  expect(psu_seq_edit_key(&ed, &seq, '1') == 0, "keys blocked");
  expect(psu_seq_edit_apply(&ed, &seq) == 0, "apply blocked");
  expect(seq.steps[0].voltage_mv == 12500U, "running did not change voltage");
  psu_seq_stop(&seq, 10, 0);
  expect(psu_seq_edit_locked(&seq) == 0, "unlocked after stop");
}

static int chg_off;
static void chg_out(int on, void *user)
{
  (void)user;
  if (!on)
    chg_off = 1;
}

static void test_charger(void)
{
  PsuCharger chg;
  PsuChgProfile profile;
  PsuChgSense sense;
  char why[64];
  psu_chg_init(&chg);
  psu_chg_profile_defaults(&profile, CHEM_NIMH);
  profile.confirmed = 1;
  memset(&sense, 0, sizeof(sense));
  sense.telemetry_ok = 1;
  sense.permit = 1;
  sense.reverse_hw = 1;
  expect(psu_chg_validate(&profile, &sense, why, sizeof(why)) == 0, "nimh blocked");
  psu_chg_profile_defaults(&profile, CHEM_LIION);
  profile.confirmed = 1;
  profile.cells = 4;
  profile.polarity_checked = 1;
  sense.reverse_hw = 0;
  sense.pack_mv = 12000;
  expect(psu_chg_start(&chg, &profile, &sense, 0) == 1, "start");
  chg.io.output = chg_out;
  chg.io.limits = on_limits;
  psu_chg_tick(&chg, &sense, 10);
  expect(chg.state == CHG_WAIT || chg.state == CHG_PRECHARGE, "connected");
  sense.pack_mv = 11000;
  psu_chg_tick(&chg, &sense, 20);
  expect(chg.state == CHG_PRECHARGE, "precharge");
  sense.pack_mv = 13000;
  psu_chg_tick(&chg, &sense, 20);
  psu_chg_tick(&chg, &sense, 600);
  expect(chg.state == CHG_CC, "cc");
  sense.pack_mv = 16800;
  psu_chg_tick(&chg, &sense, 700);
  psu_chg_tick(&chg, &sense, 1300);
  expect(chg.state == CHG_CV, "cv");
  sense.current_ma = 50;
  psu_chg_tick(&chg, &sense, 1400);
  psu_chg_tick(&chg, &sense, 2000);
  psu_chg_tick(&chg, &sense, 2100);
  expect(chg.state == CHG_COMPLETE, "complete");
  psu_chg_init(&chg);
  psu_chg_start(&chg, &profile, &sense, 0);
  chg.io.output = chg_out;
  chg_off = 0;
  psu_chg_tick(&chg, &sense, 10);
  sense.telemetry_ok = 0;
  psu_chg_tick(&chg, &sense, 20);
  expect(chg.state == CHG_FAULT, "telemetry fault");
  expect(chg_off == 1, "priority off");

  psu_chg_init(&chg);
  psu_chg_profile_defaults(&profile, CHEM_LIION);
  profile.cells = 0;
  expect(psu_chg_max_cells(&profile) == 6U, "liion max cells");
  expect(psu_chg_user_start(&chg, &profile, &sense, 0, 0, 0, 0) == 0, "no profile");
  expect(strcmp(chg.reason, "SELECT A PROFILE") == 0, "profile text");
  profile.cells = 0;
  expect(psu_chg_user_start(&chg, &profile, &sense, 0, 1, 0, 0) == 0, "no cells");
  expect(strcmp(chg.reason, "CELL COUNT REQUIRED") == 0, "cells text");
  profile.cells = 7;
  expect(psu_chg_user_start(&chg, &profile, &sense, 0, 1, 1, 1) == 0, "cells over 27V");
  profile.cells = 4;
  profile.confirmed = 0;
  profile.polarity_checked = 0;
  expect(psu_chg_user_start(&chg, &profile, &sense, 0, 1, 1, 0) == 1, "polarity latch");
  expect(chg.running == 0U && profile.confirmed == 0U, "confirm is not a start");
  expect(strcmp(chg.reason, "POLARITY CONFIRMED. PRESS START") == 0, "confirm text");
  sense.telemetry_ok = 1;
  sense.permit = 1;
  sense.pack_mv = 12000;
  expect(psu_chg_user_start(&chg, &profile, &sense, 0, 1, 1, 1) == 2, "start after confirm");
  expect(chg.state == CHG_VALIDATE, "validated");
}

static void test_store_and_app(void)
{
  uint8_t a[8192], b[8192];
  PsuSnapshot s;
  PsuG0Sample g0;
  char json[1024];
  char page[128];
  size_t off = 0;
  size_t n;
  uint8_t ip[4] = {192, 168, 1, 40};
  uint8_t mask[4] = {255, 255, 255, 0};
  uint8_t gw[4] = {192, 168, 1, 1};
  uint8_t zero[4] = {0, 0, 0, 0};
  int flap;
  const PsuPreset *preset;
  memset(a, 0, sizeof(a));
  memset(b, 0, sizeof(b));
  psu_app_init();
  psu_snapshot(&s);
  expect(s.output_requested == 0 && s.output_confirmed == 0, "cold off");
  expect(s.cold_output_off == 1, "cold flag");
  expect(psu_app_set_limits(12000, 2000, PSU_SRC_LCD) == 1, "lcd limits");
  psu_snapshot(&s);
  expect(s.requested_mv == 12000 && s.requested_ma == 2000, "requested");
  expect(psu_web_write_state(json, sizeof(json)) > 0, "json");
  expect(strstr(json, "\"requested_mv\":12000") != 0, "json mv");
  expect(psu_web_request("POST", "/api/command",
                         "{\"voltage_mv\":5000,\"current_ma\":1000}",
                         "browser", json, sizeof(json)) > 0, "web post");
  psu_snapshot(&s);
  expect(s.requested_mv == 5000 && s.requested_ma == 1000, "web reached model");
  expect(psu_web_request("POST", "/api/command",
                         "{\"voltage_mv\":9000,\"current_ma\":1000}",
                         "other", json, sizeof(json)) > 0, "second client");
  expect(strstr(json, "lease") != 0, "lease reject");
  psu_config_bind_ram(a, b, sizeof(a));
  expect(psu_config_save() == 1, "save a");
  expect(psu_config_save() == 1, "save b");
  a[20] ^= 0xFF;
  expect(psu_config_load() == 1, "load other copy");
  psu_snapshot(&s);
  expect(s.output_requested == 0, "load keeps output off");
  preset = psu_preset_get(0);
  expect(preset && preset->voltage_mv == 5000, "default preset");
  expect(psu_preset_apply(1) == 1, "apply 12V");
  psu_snapshot(&s);
  expect(s.requested_mv == 12000, "preset apply");
  expect(s.output_requested == 0 && s.output_confirmed == 0, "preset does not enable output");
  expect(psu_preset_save_current(4, "USER") == 1, "save slot 5");
  expect(psu_preset_get(4) && psu_preset_get(4)->voltage_mv == 12000, "saved slot");
  expect(psu_preset_duplicate(4, 5) == 1, "duplicate slot");
  expect(psu_preset_get(5) && psu_preset_get(5)->output_action == 0, "duplicate keeps output off");

  memset(&g0, 0, sizeof(g0));
  g0.connected = 1;
  g0.current_valid = 1;
  g0.vout_mv = 5000;
  g0.iout_ua = 1000000;
  g0.applied_voltage_mv = 12000;
  g0.applied_current_ma = 2000;
  g0.status_flags = 0x0011;
  g0.maximum_voltage_mv = 27000;
  g0.maximum_current_ma = 5000;
  psu_app_observe_g0(&g0, 1000);
  psu_app_tick(1000);
  psu_app_tick(1600);
  psu_snapshot(&s);
  expect(s.output_confirmed == 1, "confirmed from status bit");
  expect(s.power_mw == 5000U, "5V * 1A");
  expect(s.last_cmd_state == PSU_CMD_ACK || s.last_cmd_state == PSU_CMD_SENT ||
             s.last_cmd_state == PSU_CMD_TIMEOUT,
         "command tracked");

  psu_net_down();
  for (flap = 0; flap < 5; ++flap)
  {
    psu_net_up();
    psu_snapshot(&s);
    expect(s.net_state == PSU_NET_DHCP && s.ip[0] == 0, "dhcp without fake ip");
    expect(psu_net_bound(zero, mask, gw) == 0, "reject zero");
    expect(psu_net_bound(ip, mask, gw) == 1, "bound");
    psu_net_down();
    psu_snapshot(&s);
    expect(s.net_state == PSU_NET_DOWN && s.ip[0] == 0, "down clears ip");
  }
  psu_net_up();
  psu_net_dhcp_timeout();
  psu_snapshot(&s);
  expect(s.net_state == PSU_NET_TIMEOUT && s.ip[0] == 0, "dhcp timeout");

  off = 0;
  n = psu_http_next("abcdef", 6, &off, page, 4);
  expect(n == 4 && memcmp(page, "abcd", 4) == 0, "chunk 1");
  n = psu_http_next("abcdef", 6, &off, page, 4);
  expect(n == 2 && off == 6, "chunk 2");

  expect(psu_app_bms_cmd("BMS") == 1, "bms command");
  expect(psu_app_usb_role("SINK", PSU_SRC_LCD) == 1, "usb sink");
  expect(psu_app_usb_role("SOURCE", PSU_SRC_LCD) == 1, "usb source");
  expect(psu_app_usb_role("AUTO", PSU_SRC_LCD) == 1, "usb auto");
  psu_render_bms(json, sizeof(json));
  expect(strstr(json, "OFFLINE") != 0, "bms hidden");
  psu_render_usb(json, sizeof(json));
  expect(strstr(json, "LOCKED") != 0, "pps locked text");
  psu_render_network(json, sizeof(json));
  expect(strstr(json, "1.50") == 0, "no fake address");
}

static void click_button(uint32_t t)
{
  psu_app_user_button(1, t);
  psu_app_user_button(1, t + 30U);
  psu_app_user_button(0, t + 80U);
  psu_app_user_button(0, t + 120U);
}

static uint8_t led_seen;

static void test_led(uint8_t on)
{
  led_seen = on;
}

static void test_button_energy_pd_bms(void)
{
  PsuSnapshot s;
  PsuG0Sample g0;
  PsuEnergySession energy;
  PsuBmsFrame frame;
  PsuBmsFace face;
  PsuUsbFace usb;
  PsuPdText pd;
  uint32_t fixed_5v = (100U << 10) | 300U;
  uint32_t apdo = (3U << 30) | (110U << 17) | (33U << 8) | 60U;
  uint32_t rdo = (2U << 28) | (1U << 25) | (150U << 10) | 300U;
  uint32_t words[2];
  uint32_t pps_rdo;
  char json[1024];
  clock_t t0;
  int i;
  PsuPdo decoded;
  PsuRdo decoded_rdo;

  psu_app_init();
  psu_app_set_output_led(test_led);
  expect(led_seen == 0, "led off at boot");
  click_button(1000);
  psu_snapshot(&s);
  expect(s.output_requested == 1, "short click requests on");
  psu_app_tick(1200);
  expect(led_seen == 0, "led ignores the request");
  click_button(2000);
  psu_snapshot(&s);
  expect(s.output_requested == 0, "second click requests off");

  expect(psu_app_set_output(1, PSU_SRC_LCD) == 1, "output on before hold");
  expect(psu_seq_start(psu_sequencer(), 3000) == 1, "seq running");
  psu_app_user_button(1, 4000);
  psu_app_user_button(1, 4030);
  psu_app_user_button(1, 6000);
  psu_app_user_button(1, 6500);
  expect(psu_app_shutdown_count() == 1U, "hold shutdown once");
  psu_snapshot(&s);
  expect(s.output_requested == 0, "shutdown requests off");
  expect(psu_sequencer()->run == PSU_SEQ_IDLE, "shutdown stops sequencer");
  expect(psu_charger()->state == CHG_IDLE, "idle charger stays idle");
  psu_app_user_button(0, 7000);
  psu_app_user_button(0, 7040);
  psu_snapshot(&s);
  expect(s.output_requested == 0, "release after hold does not click");
  expect(psu_app_shutdown_count() == 1U, "release does not shutdown again");

  psu_app_user_button(1, 8000);
  psu_app_user_button(0, 8010);
  psu_app_user_button(0, 8020);
  psu_snapshot(&s);
  expect(s.output_requested == 0, "bounce is not a click");

  memset(&g0, 0, sizeof(g0));
  g0.connected = 1;
  g0.current_valid = 1;
  g0.vout_mv = 28000;
  psu_app_observe_g0(&g0, 9000);
  psu_app_tick(9000);
  psu_snapshot(&s);
  expect(s.fault_latched == 1, "ovp latched");
  click_button(9100);
  psu_snapshot(&s);
  expect(s.output_requested == 0, "fault blocks the button");

  memset(&g0, 0, sizeof(g0));
  g0.connected = 1;
  g0.current_valid = 1;
  g0.status_flags = 0x0011;
  g0.vout_mv = 5000;
  psu_app_init();
  psu_app_set_output_led(test_led);
  psu_app_observe_g0(&g0, 10000);
  expect(led_seen == 1, "led follows confirmed on");
  g0.connected = 0;
  g0.status_flags = 0;
  psu_app_observe_g0(&g0, 11000);
  expect(led_seen == 0, "led follows confirmed off");

  psu_energy_reset(&energy);
  psu_energy_observe(&energy, 0, 1, 0, 0);
  psu_energy_observe(&energy, 1000, 1, 1, 24000);
  psu_energy_observe(&energy, 2500, 1, 1, 24000);
  expect(energy.out_mwh == 10U && energy.in_mwh == 0U, "10 mWh out");
  psu_energy_observe(&energy, 3000, 1, 0, 24000);
  psu_energy_observe(&energy, 4200, 1, 1, 24000);
  expect(energy.out_mwh == 10U && energy.gap_count == 1U, "gap adds no energy");
  psu_energy_observe(&energy, 5000, 0, 0, 0);
  expect(energy.active == 0U && energy.has_closed == 1U, "disconnect closes");
  expect(energy.closed_out_mwh == 10U, "closed energy kept");
  psu_energy_observe(&energy, 6000, 1, 1, -24000);
  psu_energy_observe(&energy, 7500, 1, 1, -24000);
  expect(energy.in_mwh == 10U && energy.out_mwh == 0U, "new session energy in");
  expect(energy.closed_out_mwh == 10U, "previous session stays");
  psu_energy_observe(&energy, 9000, 1, 1, -24000);
  expect(energy.in_mwh == 20U, "1500 ms still counts");
  psu_energy_observe(&energy, 9000U + 1501U, 1, 1, -24000);
  expect(energy.in_mwh == 20U && energy.gap_count == 1U, "1501 ms is a gap");

  psu_app_init();
  psu_usb_face(&usb);
  expect(strcmp(usb.pdo, PSU_PD_UNAVAILABLE) == 0, "no invented pdo rows");
  expect(strstr(usb.pdo, "FIXED") == 0, "empty list has no fixed row");
  expect(usb.selected_row < 0, "nothing selected");
  expect(strcmp(usb.confirmed, "UNAVAILABLE") == 0, "no confirmed role");
  expect(strstr(usb.contract, "NO PORT SAMPLES") != 0, "no fake session");
  expect(usb.fill_w == 0, "no fake flow");
  psu_app_usb_sample(0, 1, 1, 20000);
  psu_app_usb_sample(3600, 1, 1, 20000);
  psu_usb_face(&usb);
  expect(strstr(usb.flow, "FLOW OUT") != 0, "flow follows the sample");
  expect(usb.fill_w > 0 && usb.fill_x == 400, "outward bar");
  psu_app_usb_sample(4000, 0, 0, 0);
  psu_usb_face(&usb);
  expect(strstr(usb.contract, "LAST") != 0, "disconnect keeps the session");
  expect(usb.fill_w == 0, "disconnect clears the bar");

  expect(psu_pd_decode_pdo(fixed_5v, &decoded) == 1, "decode fixed");
  expect(decoded.type == 1U && decoded.vmax_mv == 5000U && decoded.imax_ma == 3000U, "5 V 3 A");
  expect(psu_pd_decode_pdo(apdo, &decoded) == 1, "decode apdo");
  expect(decoded.type == 4U && decoded.vmin_mv == 3300U && decoded.vmax_mv == 11000U, "pps range");
  expect(decoded.imax_ma == 3000U, "pps current");
  words[0] = fixed_5v;
  words[1] = apdo;
  psu_pd_describe(words, 2, 1, rdo, &pd);
  expect(pd.count == 2U && pd.selected == 1, "second object selected");
  expect(pd.line[1][0] == '>', "arrow on selected row");
  expect(pd.line[0][0] == ' ', "other row has no arrow");
  expect(strstr(pd.line[0], "5.000 V") != 0, "fixed voltage");
  expect(strstr(pd.rdo, "OBJ 2") != 0 && strstr(pd.rdo, "USBCOM") != 0, "rdo flags");
  pps_rdo = (1U << 28) | (450U << 9) | 40U;
  psu_pd_describe(&apdo, 1, 1, pps_rdo, &pd);
  expect(psu_pd_decode_rdo(pps_rdo, 1, &decoded_rdo) == 1, "pps rdo");
  expect(decoded_rdo.voltage_mv == 9000U && decoded_rdo.operating_ma == 2000U, "9 V 2 A request");
  expect(strstr(pd.rdo, "9.000 V") != 0, "pps rdo text");
  psu_pd_describe(0, 0, 1, rdo, &pd);
  expect(pd.count == 0U && strstr(pd.rdo, "PDO LIST MISSING") != 0, "rdo without pdo");
  psu_app_pd_words(words, 2, 1, rdo);
  psu_usb_face(&usb);
  expect(usb.selected_row == 1, "face keeps the selected row");
  expect(strstr(usb.contract, "CONFIRMED CONTRACT: UNAVAILABLE") != 0, "contract not confirmed");
  psu_app_pd_words(0, 0, 0, 0);

  memset(&frame, 0, sizeof(frame));
  psu_bms_face(&frame, &face);
  expect(strcmp(face.channel[0], "MISSING") == 0, "ch1 missing");
  expect(strcmp(face.channel[1], "MISSING") == 0, "ch2 missing");
  expect(strstr(face.channels_line, "0.000") == 0, "missing is not 0 V");
  expect(strstr(face.summary, "UNAVAILABLE") != 0, "soc unavailable");
  frame.valid = 1;
  frame.present[0] = 1;
  frame.cell_mv[0] = 0;
  frame.present[1] = 0;
  frame.cell_mv[1] = 3700;
  frame.present[2] = 1;
  frame.cell_mv[2] = 3650;
  frame.present[3] = 1;
  frame.stale[3] = 1;
  frame.cell_mv[3] = 3600;
  frame.fet_dsg = 1;
  frame.soc_valid = 1;
  frame.soc_pct = 80;
  frame.current_valid = 1;
  frame.current_ma = 1000;
  frame.temp_centi[0] = 250;
  frame.temp_centi[1] = 260;
  frame.temp_centi[2] = 240;
  frame.balance_mask = 0x1;
  psu_bms_face(&frame, &face);
  expect(strcmp(face.channel[0], "0.000 V") == 0, "reported zero stays on ch1");
  expect(strcmp(face.channel[1], "MISSING") == 0, "unused ch2 is missing");
  expect(strcmp(face.channel[2], "3.650 V") == 0, "ch3 not renumbered");
  expect(strcmp(face.channel[3], "STALE") == 0, "stale ch4");
  expect(strcmp(face.pack_v, "3.650 V") == 0, "pack skips missing and stale");
  expect(strcmp(face.spread, "3650 mV") == 0, "spread uses live cells");
  expect(strcmp(face.state, "DISCHARGE") == 0, "discharge fet");
  expect(strcmp(face.balance, "CH1") == 0, "balance channel");
  expect(strstr(face.channels_line, "CH1") != 0 && strstr(face.channels_line, "CH4") != 0, "channel names stay");
  psu_app_init();
  psu_app_bms_frame(&frame);
  psu_render_bms(json, sizeof(json));
  expect(strstr(json, "CH2 MISSING") != 0, "render keeps ch2");
  expect(strstr(json, "0.000") != 0, "render shows a real zero");

  expect(psu_swipe_back(1, 8, 100, 120, 110, 1, 4, 0) == 1, "edge swipe back");
  expect(psu_swipe_back(0, 8, 100, 120, 110, 1, 4, 0) == 0, "unarmed swipe");
  expect(psu_swipe_back(1, 8, 200, 120, 210, 1, 4, 1) == 0, "blocked editor");
  expect(psu_swipe_back(1, 40, 100, 160, 110, 1, 4, 0) == 0, "swipe not from the edge");
  expect(psu_swipe_back(1, 8, 100, 40, 110, 1, 4, 0) == 0, "short swipe");
  expect(psu_swipe_back(1, 8, 100, 120, 200, 1, 4, 0) == 0, "vertical drag");
  expect(psu_swipe_back(1, 8, 100, 120, 110, 0, 4, 0) == 0, "vertical gesture");
  expect(psu_swipe_back(1, 8, 100, 20, 110, 1, -4, 0) == 0, "leftward swipe");

  words[0] = fixed_5v;
  words[1] = apdo;
  t0 = clock();
  for (i = 0; i < 20000; ++i)
    psu_pd_describe(words, 2, 1, rdo, &pd);
  printf("pdo describe %.2f us\n",
         (double)(clock() - t0) * 1000000.0 / (double)CLOCKS_PER_SEC / 20000.0);
  t0 = clock();
  for (i = 0; i < 20000; ++i)
    psu_bms_face(&frame, &face);
  printf("bms face %.2f us\n",
         (double)(clock() - t0) * 1000000.0 / (double)CLOCKS_PER_SEC / 20000.0);
  t0 = clock();
  for (i = 0; i < 20000; ++i)
    psu_energy_observe(&energy, (uint32_t)(20000 + i), 1, 1, 1000);
  printf("energy observe %.2f us\n",
         (double)(clock() - t0) * 1000000.0 / (double)CLOCKS_PER_SEC / 20000.0);
}

int main(void)
{
  test_g0();
  test_format_edit();
  test_g4();
  test_seq();
  test_seq_edit();
  test_charger();
  test_store_and_app();
  test_button_energy_pd_bms();
  if (failures)
  {
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
  }
  printf("psu host tests passed\n");
  return 0;
}
