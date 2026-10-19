#include "psu_app.h"
#include "psu_format.h"
#include <stdio.h>
#include <string.h>

static void link_name(char *dst, size_t n, uint8_t link)
{
  const char *text = "OFFLINE";
  if (link == G4_LINK_ONLINE)
    text = "ONLINE";
  else if (link == G4_LINK_STALE)
    text = "STALE";
  snprintf(dst, n, "%s", text);
}

void psu_render_diagnostics(char *dst, size_t n)
{
  PsuSnapshot s;
  char g4[16];
  char request_v[16];
  char request_a[16];
  char applied[40];
  const char *output;
  if (dst == 0 || n == 0U)
    return;
  psu_snapshot(&s);
  link_name(g4, sizeof(g4), s.g4_link);
  psu_format_voltage(request_v, sizeof(request_v), s.requested_mv);
  psu_format_current_ma(request_a, sizeof(request_a), s.requested_ma);
  if (s.applied_valid)
  {
    char av[16];
    char aa[16];
    psu_format_voltage(av, sizeof(av), s.applied_mv);
    psu_format_current_ma(aa, sizeof(aa), s.applied_ma);
    snprintf(applied, sizeof(applied), "%s  %s", av, aa);
  }
  else
    snprintf(applied, sizeof(applied), "UNAVAILABLE");
  if (s.output_confirmed)
    output = "CONFIRMED";
  else if (s.output_requested)
    output = "REQUESTED ONLY";
  else
    output = "OFF";
  snprintf(dst, n,
           "REQUEST %s  %s\n"
           "APPLIED %s\n"
           "OUTPUT %s\n"
           "G0 %s  CRC %u\n"
           "G4 %s  UART %s\n"
           "TX %s\n"
           "RX %s\n"
           "LD1 FOLLOWS CONFIRMED OUTPUT",
           request_v, request_a, applied, output,
           s.g0_connected ? (s.g0_stale ? "STALE" : "ONLINE") : "OFFLINE",
           s.g0_crc_errors,
           g4, s.g4_uart_configured ? "READY" : "NOT CONFIGURED",
           s.g4_last_tx[0] ? s.g4_last_tx : "-",
           s.g4_last_rx[0] ? s.g4_last_rx : "-");
}

void psu_appearance_face(PsuAppearanceFace *out)
{
  if (out == 0)
    return;
  memset(out, 0, sizeof(*out));
  snprintf(out->display, sizeof(out->display), "800 x 480");
  snprintf(out->typeface, sizeof(out->typeface), "IBM PLEX");
  snprintf(out->sound, sizeof(out->sound), "NONE");
  snprintf(out->stored, sizeof(out->stored), "NOT IN EEPROM");
  snprintf(out->note, sizeof(out->note),
           "Presets, steps and limits are stored.\n"
           "Output is not restored on.\n"
           "A running sequence or charge is not restored.\n"
           "Appearance and sound are not stored.");
}

void psu_render_bms(char *dst, size_t n)
{
  PsuBmsFrame frame;
  PsuBmsFace face;
  psu_bms_read(&frame);
  psu_bms_face(&frame, &face);
  if (!frame.valid)
  {
    snprintf(dst, n,
             "BMS OFFLINE\n%s\n"
             "No proven BQ76922 frame in this firmware.\n"
             "Commands BMS, BMS OFF and CLR are queued to G4.\n"
             "Missing channels stay missing. They are not 0 V.",
             face.channels_line);
    return;
  }
  snprintf(dst, n, "%s\n%s\n%s", face.state, face.channels_line, face.summary);
}

void psu_render_battery(char *dst, size_t n)
{
  PsuSnapshot s;
  psu_snapshot(&s);
  snprintf(dst, n,
           "INTERNAL PACK\n%s\n"
           "This is the instrument battery, not the external charger.\n"
           "SOC and capacity stay blank until BMS telemetry is valid.",
           s.bms_valid ? "TELEMETRY VALID" : "OFFLINE");
}

void psu_render_usb(char *dst, size_t n)
{
  PsuUsbFace face;
  psu_usb_face(&face);
  snprintf(dst, n,
           "REQUESTED %s\nCONFIRMED %s\nLINK %s\nPPS %s\n%s\n%s\n%s",
           face.requested, face.confirmed, face.link, face.pps,
           face.flow, face.pdo, face.contract);
}

void psu_render_measurements(char *dst, size_t n)
{
  PsuSnapshot s;
  char v[24], i[24], p[24];
  psu_snapshot(&s);
  psu_format_voltage(v, sizeof(v), s.vout_mv);
  psu_format_current_ua(i, sizeof(i), s.display_current_ua);
  psu_format_power_mw(p, sizeof(p), s.power_mw);
  snprintf(dst, n,
           "DATA %s\nVIN %u mV\nVOUT %s\nIOUT %s\nPOWER %s\n"
           "Signed IOUT %d uA is kept for diagnostics.",
           s.g0_connected ? (s.g0_stale ? "STALE" : "VALID") : "OFFLINE",
           s.vin_mv, v, i, p, (int)s.signed_current_ua);
}

void psu_render_protection(char *dst, size_t n)
{
  PsuSnapshot s;
  psu_snapshot(&s);
  snprintf(dst, n,
           "LIMITS 0-27.000 V  0-5.000 A\n"
           "FAULT %s\n%s\n"
           "Clear fault sends CLR and stays latched until confirmed.\n"
           "LD1 follows the confirmed output, not the request.",
           s.fault_latched ? "LATCHED" : "CLEAR",
           s.fault);
}

void psu_render_network(char *dst, size_t n)
{
  PsuSnapshot s;
  const char *st = "LINK DOWN";
  psu_snapshot(&s);
  if (!s.ethernet_configured)
    st = "NOT IN BUILD";
  else if (s.net_state == PSU_NET_DHCP) st = "DHCP";
  else if (s.net_state == PSU_NET_BOUND) st = "BOUND";
  else if (s.net_state == PSU_NET_TIMEOUT) st = "DHCP TIMEOUT";
  snprintf(dst, n,
           "%s\nIP %u.%u.%u.%u\nMASK %u.%u.%u.%u\nGW %u.%u.%u.%u\n"
           "MAC %02X:%02X:%02X:%02X:%02X:%02X\n"
           "Ethernet IP is not enabled in CubeMX. No address is invented.",
           st, s.ip[0], s.ip[1], s.ip[2], s.ip[3],
           s.mask[0], s.mask[1], s.mask[2], s.mask[3],
           s.gateway[0], s.gateway[1], s.gateway[2], s.gateway[3],
           s.mac[0], s.mac[1], s.mac[2], s.mac[3], s.mac[4], s.mac[5]);
}

void psu_render_sequencer(char *dst, size_t n)
{
  PsuSequencer *seq = psu_sequencer();
  PsuSnapshot s;
  unsigned i;
  int used;
  psu_snapshot(&s);
  used = snprintf(dst, n, "%s  STEPS %u  LOCK %u\n",
                  s.seq_status, seq->count, seq->config_locked);
  if (used < 0 || (size_t)used >= n)
    return;
  for (i = 0; i < seq->count && (size_t)used + 48U < n; ++i)
  {
    int line = snprintf(dst + used, n - (size_t)used,
                        "%c%u %u.%03uV %u.%03uA %ums %s\n",
                        (i == seq->selected) ? '>' : ' ',
                        i + 1U,
                        seq->steps[i].voltage_mv / 1000U, seq->steps[i].voltage_mv % 1000U,
                        seq->steps[i].current_ma / 1000U, seq->steps[i].current_ma % 1000U,
                        seq->steps[i].time_ms,
                        seq->steps[i].enabled ? "ON" : "SKIP");
    if (line < 0)
      break;
    used += line;
  }
}

void psu_render_charger(char *dst, size_t n)
{
  PsuCharger *chg = psu_charger();
  snprintf(dst, n,
           "EXTERNAL BATTERY\n%s\n%s\n"
           "Cells %u  CV/cell %u mV  CC %u mA\n"
           "Confirm chemistry, series count and polarity before start.\n"
           "Internal BMS is a different screen.",
           psu_chg_state_name(chg->state), chg->reason,
           chg->profile.cells, chg->profile.cv_mv_cell, chg->profile.cc_ma);
}

void psu_render_presets(char *dst, size_t n)
{
  unsigned i;
  int used = snprintf(dst, n, "PRESETS\n");
  for (i = 0; i < PSU_PRESET_COUNT && (size_t)used + 40U < n; ++i)
  {
    const PsuPreset *p = psu_preset_get((uint8_t)i);
    int line;
    if (p == 0)
      break;
    line = snprintf(dst + used, n - (size_t)used, "%u %s  %u mV  %u mA\n",
                    i + 1U, p->name, p->voltage_mv, p->current_ma);
    if (line < 0)
      break;
    used += line;
  }
}

void psu_render_service(char *dst, size_t n)
{
  PsuSnapshot s;
  psu_snapshot(&s);
  snprintf(dst, n,
           "SERVICE MODE %s\n"
           "Manual lines bypass some GUI checks.\n"
           "Last TX %s\nLast RX %s",
           s.service_mode ? "ON" : "OFF",
           s.g4_last_tx[0] ? s.g4_last_tx : "-",
           s.g4_last_rx[0] ? s.g4_last_rx : "-");
}

void psu_render_power_path(char *dst, size_t n)
{
  PsuSnapshot s;
  psu_snapshot(&s);
  snprintf(dst, n,
           "CHARGER AND POWER PATH\n"
           "Permit and remote sense use PERMIT and REMOTE commands.\n"
           "Requested %u mV / %u mA\nApplied valid %u\nFault %s",
           s.requested_mv, s.requested_ma, s.applied_valid, s.fault);
}
