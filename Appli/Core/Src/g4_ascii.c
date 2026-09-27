#include "g4_ascii.h"
#include "psu_limits.h"
#include <stdio.h>
#include <string.h>

static void log_line(G4Port *port, int tx, const char *text, uint32_t ms)
{
  G4RawLine *slot;
  if (port == 0 || text == 0)
    return;
  slot = &port->log[port->log_pos];
  memset(slot, 0, sizeof(*slot));
  (void)snprintf(slot->text, sizeof(slot->text), "%s", text);
  slot->dir_tx = tx ? 1U : 0U;
  slot->ms = ms;
  port->log_pos = (uint8_t)((port->log_pos + 1U) % G4_RAW_LOG);
  if (port->log_count < G4_RAW_LOG)
    ++port->log_count;
}

static int queue_push(G4Port *port, const G4Slot *slot)
{
  if (port->count >= G4_QUEUE_LEN)
    return 0;
  port->queue[port->head] = *slot;
  port->head = (uint8_t)((port->head + 1U) % G4_QUEUE_LEN);
  ++port->count;
  return 1;
}

static int queue_push_safety(G4Port *port, const G4Slot *slot)
{
  if (port->count < G4_QUEUE_LEN)
    return queue_push(port, slot);
  /* Drop the oldest normal command so OFF/CLR still fit. */
  if (port->count > 0U)
  {
    port->tail = (uint8_t)((port->tail + 1U) % G4_QUEUE_LEN);
    --port->count;
  }
  return queue_push(port, slot);
}

void g4_init(G4Port *port)
{
  if (port == 0)
    return;
  memset(port, 0, sizeof(*port));
  port->link = G4_LINK_OFFLINE;
  port->next_id = 1U;
}

void g4_link_up(G4Port *port)
{
  if (port == 0)
    return;
  port->link = G4_LINK_ONLINE;
  port->need_session_init = 1U;
  port->session_ready = 0U;
  port->init_phase = 0U;
  port->last_rx_ms = 0U;
}

void g4_link_lost(G4Port *port)
{
  if (port == 0)
    return;
  port->link = G4_LINK_OFFLINE;
  port->session_ready = 0U;
  port->need_session_init = 0U;
  port->init_phase = 0U;
}

static int enqueue_text(G4Port *port, const char *cmd, int safety, uint16_t cmd_id)
{
  G4Slot slot;
  size_t len;
  if (port == 0 || cmd == 0 || cmd[0] == '\0')
    return 0;
  len = strlen(cmd);
  if (len == 0U || len >= sizeof(slot.text))
    return 0;
  memset(&slot, 0, sizeof(slot));
  memcpy(slot.text, cmd, len);
  slot.safety = safety ? 1U : 0U;
  slot.cmd_id = cmd_id;
  if (safety)
    return queue_push_safety(port, &slot);
  return queue_push(port, &slot);
}

int g4_enqueue(G4Port *port, const char *cmd, int safety, uint16_t cmd_id)
{
  return enqueue_text(port, cmd, safety, cmd_id);
}

int g4_set_mv(G4Port *port, uint32_t voltage_mv, uint16_t cmd_id)
{
  char line[40];
  int n;
  if (port == 0)
    return 0;
  voltage_mv = psu_clamp_voltage_mv(voltage_mv);
  n = snprintf(line, sizeof(line), "SET %u.%03u",
               (unsigned)(voltage_mv / 1000U), (unsigned)(voltage_mv % 1000U));
  if (n <= 0 || (size_t)n >= sizeof(line))
    return 0;
  (void)snprintf(port->pending_set, sizeof(port->pending_set), "%s", line);
  port->has_set = 1U;
  port->set_id = cmd_id;
  return 1;
}

int g4_ilim_ma(G4Port *port, uint32_t current_ma, uint16_t cmd_id)
{
  char line[40];
  int n;
  if (port == 0)
    return 0;
  current_ma = psu_clamp_current_ma(current_ma);
  n = snprintf(line, sizeof(line), "ILIM %u.%03u",
               (unsigned)(current_ma / 1000U), (unsigned)(current_ma % 1000U));
  if (n <= 0 || (size_t)n >= sizeof(line))
    return 0;
  (void)snprintf(port->pending_ilim, sizeof(port->pending_ilim), "%s", line);
  port->has_ilim = 1U;
  port->ilim_id = cmd_id;
  return 1;
}

int g4_off(G4Port *port, uint16_t cmd_id)
{
  return enqueue_text(port, "OFF", 1, cmd_id);
}

int g4_on(G4Port *port, uint16_t cmd_id)
{
  return enqueue_text(port, "ON", 0, cmd_id);
}

int g4_usb_role(G4Port *port, const char *role, uint16_t cmd_id)
{
  char line[24];
  if (role == 0)
    return 0;
  if (strcmp(role, "AUTO") != 0 && strcmp(role, "SINK") != 0 && strcmp(role, "SOURCE") != 0)
    return 0;
  (void)snprintf(line, sizeof(line), "USB %s", role);
  return enqueue_text(port, line, 0, cmd_id);
}

int g4_pps(G4Port *port, uint32_t voltage_mv, uint32_t current_ma, uint16_t cmd_id)
{
  char line[40];
  int n;
  if (port == 0 || !g4_pps_allowed(port))
    return 0;
  voltage_mv = psu_clamp_voltage_mv(voltage_mv);
  current_ma = psu_clamp_current_ma(current_ma);
  n = snprintf(line, sizeof(line), "PPS %u.%02u %u.%02u",
               (unsigned)(voltage_mv / 1000U), (unsigned)((voltage_mv % 1000U) / 10U),
               (unsigned)(current_ma / 1000U), (unsigned)((current_ma % 1000U) / 10U));
  if (n <= 0 || (size_t)n >= sizeof(line))
    return 0;
  (void)snprintf(port->pending_pps, sizeof(port->pending_pps), "%s", line);
  port->has_pps = 1U;
  port->pps_id = cmd_id;
  return 1;
}

int g4_permit(G4Port *port, int enabled, uint16_t cmd_id)
{
  return enqueue_text(port, enabled ? "PERMIT 1" : "PERMIT 0", 0, cmd_id);
}

int g4_remote(G4Port *port, int enabled, uint16_t cmd_id)
{
  return enqueue_text(port, enabled ? "REMOTE 1" : "REMOTE 0", 0, cmd_id);
}

int g4_verbose(G4Port *port, int enabled, uint16_t cmd_id)
{
  return enqueue_text(port, enabled ? "VERBOSE 1" : "VERBOSE 0", 0, cmd_id);
}

int g4_simple(G4Port *port, const char *cmd, uint16_t cmd_id)
{
  static const char *allowed[] = {
      "STATUS", "BMS", "BMS OFF", "CLR", "TEL 0", "TEL 100", "TEL 250",
      "TEL 500", "TEL 1000", "TEL 2000", "ON", "OFF"};
  size_t i;
  int safety = 0;
  if (cmd == 0)
    return 0;
  for (i = 0; i < sizeof(allowed) / sizeof(allowed[0]); ++i)
  {
    if (strcmp(cmd, allowed[i]) == 0)
    {
      safety = (strcmp(cmd, "OFF") == 0 || strcmp(cmd, "CLR") == 0);
      return enqueue_text(port, cmd, safety, cmd_id);
    }
  }
  return 0;
}

int g4_service_line(G4Port *port, const char *cmd, uint16_t cmd_id)
{
  size_t len;
  if (port == 0 || !port->service_mode || cmd == 0)
    return 0;
  len = strlen(cmd);
  if (len == 0U || len > 40U)
    return 0;
  if (strchr(cmd, '\n') != 0 || strchr(cmd, '\r') != 0)
    return 0;
  return enqueue_text(port, cmd, 0, cmd_id);
}

static int take_normal(G4Port *port, G4Slot *out)
{
  if (port->count == 0U)
    return 0;
  *out = port->queue[port->tail];
  port->tail = (uint8_t)((port->tail + 1U) % G4_QUEUE_LEN);
  --port->count;
  return 1;
}

static int take_safety_first(G4Port *port, G4Slot *out)
{
  uint8_t i;
  uint8_t idx;
  if (port->count == 0U)
    return 0;
  idx = port->tail;
  for (i = 0; i < port->count; ++i)
  {
    if (port->queue[idx].safety)
    {
      G4Slot keep[G4_QUEUE_LEN];
      uint8_t k = 0U;
      uint8_t p = port->tail;
      uint8_t c = port->count;
      *out = port->queue[idx];
      while (c--)
      {
        if (p != idx)
          keep[k++] = port->queue[p];
        p = (uint8_t)((p + 1U) % G4_QUEUE_LEN);
      }
      memset(port->queue, 0, sizeof(port->queue));
      for (p = 0; p < k; ++p)
        port->queue[p] = keep[p];
      port->tail = 0U;
      port->head = k;
      port->count = k;
      return 1;
    }
    idx = (uint8_t)((idx + 1U) % G4_QUEUE_LEN);
  }
  return take_normal(port, out);
}

void g4_process(G4Port *port, uint32_t now_ms)
{
  (void)now_ms;
  if (port == 0)
    return;
  if (port->link == G4_LINK_ONLINE && port->last_rx_ms != 0U &&
      (now_ms - port->last_rx_ms) > 1500U)
  {
    port->link = G4_LINK_STALE;
    port->session_ready = 0U;
    port->need_session_init = 1U;
    port->init_phase = 0U;
  }
}

int g4_pop_tx(G4Port *port, char *out, size_t n, uint32_t now_ms)
{
  G4Slot slot;
  int ready = 0;
  if (port == 0 || out == 0 || n < 4U)
    return 0;
  memset(&slot, 0, sizeof(slot));
  if (take_safety_first(port, &slot) && slot.safety)
    ready = 1;
  else if (slot.text[0] != '\0')
  {
    /* take_safety_first already consumed a normal slot when no safety existed
       only if it fell through. When a normal slot was taken, safety flag is 0
       and text is set. Put it back if session init must go first. */
    if (port->need_session_init)
    {
      (void)queue_push(port, &slot);
      slot.text[0] = '\0';
    }
    else
      ready = 1;
  }

  if (!ready && port->need_session_init)
  {
    const char *init = (port->init_phase == 0U) ? "TEL 500" : "STATUS";
    (void)snprintf(slot.text, sizeof(slot.text), "%s", init);
    slot.cmd_id = 0U;
    if (port->init_phase == 0U)
      port->init_phase = 1U;
    else
    {
      port->need_session_init = 0U;
      port->session_ready = 1U;
      port->init_phase = 0U;
    }
    ready = 1;
  }

  if (!ready)
  {
    int gap_ok = (port->last_pair_ms == 0U) || ((now_ms - port->last_pair_ms) >= G4_SETPOINT_GAP_MS);
    if (gap_ok && port->has_ilim)
    {
      (void)snprintf(slot.text, sizeof(slot.text), "%s", port->pending_ilim);
      slot.cmd_id = port->ilim_id;
      port->has_ilim = 0U;
      port->pair_open = 1U;
      ready = 1;
    }
    else if (gap_ok && port->has_set)
    {
      (void)snprintf(slot.text, sizeof(slot.text), "%s", port->pending_set);
      slot.cmd_id = port->set_id;
      port->has_set = 0U;
      port->last_pair_ms = now_ms;
      port->pair_open = 0U;
      ready = 1;
    }
    else if (gap_ok && port->has_pps)
    {
      (void)snprintf(slot.text, sizeof(slot.text), "%s", port->pending_pps);
      slot.cmd_id = port->pps_id;
      port->has_pps = 0U;
      port->last_pair_ms = now_ms;
      ready = 1;
    }
    else if (port->pair_open && port->has_set == 0U)
    {
      port->last_pair_ms = now_ms;
      port->pair_open = 0U;
    }
  }

  if (!ready && port->count > 0U && !port->need_session_init)
  {
    if (take_normal(port, &slot))
      ready = 1;
  }

  if (!ready)
    return 0;
  (void)snprintf(out, n, "%s\n", slot.text);
  (void)snprintf(port->last_tx, sizeof(port->last_tx), "%s", slot.text);
  ++port->tx_count;
  log_line(port, 1, slot.text, now_ms);
  return 1;
}

static void accept_line(G4Port *port, uint32_t now_ms)
{
  if (port->rx_len == 0U)
    return;
  port->rx_acc[port->rx_len] = '\0';
  ++port->rx_lines;
  port->last_rx_ms = now_ms;
  if (port->link == G4_LINK_OFFLINE)
    g4_link_up(port);
  else
    port->link = G4_LINK_ONLINE;
  /* Grammar for TC/PDO/BMS lines is not in this repo. Keep the raw text. */
  log_line(port, 0, port->rx_acc, now_ms);
  port->rx_len = 0U;
  port->rx_acc[0] = '\0';
}

void g4_rx_bytes(G4Port *port, const uint8_t *data, size_t n, uint32_t now_ms)
{
  size_t i;
  if (port == 0 || data == 0)
    return;
  port->rx_bytes += (uint32_t)n;
  for (i = 0; i < n; ++i)
  {
    uint8_t ch = data[i];
    if (ch == '\r')
      continue;
    if (ch == '\n')
    {
      accept_line(port, now_ms);
      continue;
    }
    if (port->rx_len + 1U >= G4_LINE_MAX)
    {
      ++port->overflow_count;
      port->rx_len = 0U;
      continue;
    }
    port->rx_acc[port->rx_len++] = (char)ch;
  }
}

void g4_set_pps_gates(G4Port *port, int pps_ctl, int sink, int apdo)
{
  if (port == 0)
    return;
  port->pps_ctl = pps_ctl ? 1U : 0U;
  port->sink_role = sink ? 1U : 0U;
  port->apdo_selected = apdo ? 1U : 0U;
}

int g4_pps_allowed(const G4Port *port)
{
  if (port == 0)
    return 0;
  return port->pps_ctl && port->sink_role && port->apdo_selected;
}
