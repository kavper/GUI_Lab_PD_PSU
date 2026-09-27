#include "ldo_protocol.h"
#include "g0_frame.h"
#include <string.h>

#define SOF0 0xA5U
#define SOF1 0x5AU
#define MAX_PAYLOAD 80U
#define RX_CHUNK 96U
#define CMD_QUEUE_SIZE 6U
#define CMD_TIMEOUT_MS 100U
#define MAX_RETRIES 2U

enum { TYPE_SET_OUTPUT = 0x03, TYPE_SET_LIMITS = 0x05, TYPE_GET_INFO = 0x07 };
enum { TYPE_TELEMETRY = 0x80, TYPE_ACK = 0x81, TYPE_NACK = 0x82, TYPE_INFO = 0x83 };

typedef struct
{
  uint8_t type;
  uint8_t length;
  uint8_t payload[8];
} Command;

static UART_HandleTypeDef *port;
static uint8_t rx_chunk[RX_CHUNK];
static uint8_t frame[2U + 1U + 2U + MAX_PAYLOAD + 2U];
static uint8_t frame_pos;
static uint8_t frame_total;
static uint8_t sequence;
static uint8_t tx_frame[15];
static volatile uint8_t tx_busy;
static Command queue[CMD_QUEUE_SIZE];
static volatile uint8_t queue_head;
static volatile uint8_t queue_tail;
static Command pending;
static volatile uint8_t pending_active;
static volatile uint8_t pending_result;
static uint8_t pending_seq;
static uint8_t pending_retries;
static uint32_t pending_sent_ms;
static volatile uint32_t telemetry_generation;
static LDO_Telemetry telemetry;
static uint32_t last_telemetry_ms;
static uint32_t capabilities;
static uint8_t link_was_connected;
static volatile LDO_Diagnostics diagnostics;
static int64_t vin_filter_q8;
static int64_t vout_filter_q8;
static int64_t iout_filter_q8;
static int64_t temperature_filter_q8[4];
static uint8_t filter_initialized;

static int64_t ema_q8(int64_t filtered, int32_t sample)
{
  /* Alpha = 1/4. Fixed-point Q8 avoids floating point in the UART callback. */
  return filtered + ((((int64_t)sample * 256) - filtered) / 4);
}

static uint16_t crc16(const uint8_t *data, uint16_t size)
{
  uint16_t crc = 0xFFFFU;
  while (size--)
  {
    crc ^= (uint16_t)(*data++) << 8;
    for (uint8_t bit = 0; bit < 8U; ++bit)
      crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
  }
  return crc;
}

static uint16_t le16(const uint8_t *p)
{
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put32(uint8_t *p, uint32_t value)
{
  p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static void restart_rx(void)
{
  if (port != NULL)
    (void)HAL_UARTEx_ReceiveToIdle_IT(port, rx_chunk, sizeof(rx_chunk));
}

static void publish_telemetry(const uint8_t *p)
{
  G0RawTelemetry raw;
  if (g0_decode_telemetry68(p, &raw) != 0)
    return;
  const uint32_t raw_vin_mv = raw.vin_mv;
  const uint32_t raw_vout_mv = raw.vout_mv;
  const int32_t raw_iout_ua = raw.iout_ua;
  int16_t raw_temperature[4];
  for (uint8_t i = 0; i < 4U; ++i)
    raw_temperature[i] = raw.temperature_centi_c[i];

  if (!filter_initialized)
  {
    vin_filter_q8 = (int64_t)raw_vin_mv << 8;
    vout_filter_q8 = (int64_t)raw_vout_mv << 8;
    iout_filter_q8 = (int64_t)raw_iout_ua * 256;
    for (uint8_t i = 0; i < 4U; ++i)
      temperature_filter_q8[i] = (int64_t)raw_temperature[i] * 256;
    filter_initialized = 1U;
  }
  else
  {
    vin_filter_q8 = ema_q8(vin_filter_q8, (int32_t)raw_vin_mv);
    vout_filter_q8 = ema_q8(vout_filter_q8, (int32_t)raw_vout_mv);
    iout_filter_q8 = ema_q8(iout_filter_q8, raw_iout_ua);
    for (uint8_t i = 0; i < 4U; ++i)
      temperature_filter_q8[i] = ema_q8(temperature_filter_q8[i], raw_temperature[i]);
  }

  ++telemetry_generation;
  telemetry.protocol_version = raw.protocol_version;
  telemetry.telemetry_version = raw.telemetry_version;
  telemetry.status_flags = raw.status_flags;
  telemetry.fault_flags = raw.fault_flags;
  telemetry.uptime_ms = raw.uptime_ms;
  telemetry.vin_mv = (uint32_t)((vin_filter_q8 + 128) >> 8);
  telemetry.vout_mv = (uint32_t)((vout_filter_q8 + 128) >> 8);
  telemetry.iout_ua = (int32_t)((iout_filter_q8 + (iout_filter_q8 >= 0 ? 128 : -128)) >> 8);
  for (uint8_t i = 0; i < 4U; ++i)
    telemetry.temperature_centi_c[i] =
        (int16_t)((temperature_filter_q8[i] +
                  (temperature_filter_q8[i] >= 0 ? 128 : -128)) >> 8);
  telemetry.requested_voltage_mv = raw.requested_voltage_mv;
  telemetry.iout_adc_raw = raw.iout_adc_raw;
  telemetry.dac_cv_readback_mv = raw.dac_cv_readback_mv;
  telemetry.dac_cc_readback_mv = raw.dac_cc_readback_mv;
  telemetry.requested_current_ma = raw.requested_current_ma;
  telemetry.applied_voltage_mv = raw.applied_voltage_mv;
  telemetry.applied_current_ma = raw.applied_current_ma;
  telemetry.preregulator_mv = raw.preregulator_mv;
  telemetry.mode = raw.mode;
  telemetry.startup = raw.startup;
  telemetry.reserved = raw.reserved;
  telemetry.current_calibrated = (capabilities & 1U) ? 1U : 0U;
  telemetry.current_valid = (telemetry.status_flags & (1U << 4)) ? 1U : 0U;
  telemetry.connected = 1U;
  telemetry.telemetry_stale = 0U;
  last_telemetry_ms = HAL_GetTick();
  ++telemetry_generation;
}

static void dispatch(uint8_t type, uint8_t seq, const uint8_t *payload, uint8_t payload_len)
{
  if (type == TYPE_TELEMETRY && payload_len == 68U && payload[0] == 1U && payload[1] == 1U)
    publish_telemetry(payload);
  else if (type == TYPE_INFO && payload_len == 16U)
  {
    capabilities = le32(payload + 12);
    ++telemetry_generation;
    telemetry.protocol_version = payload[0];
    telemetry.telemetry_version = payload[1];
    telemetry.telemetry_period_ms = le16(payload + 2);
    telemetry.maximum_voltage_mv = le32(payload + 4);
    telemetry.maximum_current_ma = le32(payload + 8);
    telemetry.capability_flags = capabilities;
    telemetry.current_calibrated = (capabilities & 1U) ? 1U : 0U;
    ++telemetry_generation;
    if (pending_active && seq == pending_seq && pending.type == TYPE_GET_INFO)
      pending_result = 1U;
  }
  else if (type == TYPE_ACK && payload_len == 1U)
  {
    ++diagnostics.ack_frames;
    if (pending_active && seq == pending_seq && payload[0] == pending.type)
      pending_result = 1U;
  }
  else if (type == TYPE_NACK && payload_len == 2U)
  {
    ++diagnostics.nack_frames;
    diagnostics.last_nack_reason = payload[1];
    if (pending_active && seq == pending_seq && payload[0] == pending.type)
      pending_result = 2U;
  }
}

static void parse_byte(uint8_t value)
{
  if (frame_pos == 0U)
  {
    if (value == SOF0) frame[frame_pos++] = value;
    return;
  }
  if (frame_pos == 1U)
  {
    if (value == SOF1) frame[frame_pos++] = value;
    else frame_pos = (value == SOF0) ? 1U : 0U;
    return;
  }
  frame[frame_pos++] = value;
  if (frame_pos == 3U)
  {
    if (value < 2U || value > (2U + MAX_PAYLOAD))
    {
      frame_pos = 0U;
      return;
    }
    frame_total = (uint8_t)(2U + 1U + value + 2U);
  }
  if (frame_total != 0U && frame_pos == frame_total)
  {
    const uint16_t received = le16(frame + frame_total - 2U);
    const uint16_t calculated = crc16(frame + 2U, (uint16_t)(frame[2] + 1U));
    if (received == calculated)
    {
      ++diagnostics.valid_frames;
      dispatch(frame[3], frame[4], frame + 5, (uint8_t)(frame[2] - 2U));
    }
    else
      ++diagnostics.crc_errors;
    frame_pos = 0U;
    frame_total = 0U;
  }
}

static uint8_t queue_command(const Command *command, uint8_t priority)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  uint8_t next;
  if (priority)
  {
    next = (uint8_t)((queue_tail + CMD_QUEUE_SIZE - 1U) % CMD_QUEUE_SIZE);
    if (next == queue_head) { if (!primask) __enable_irq(); return 0U; }
    queue_tail = next;
    queue[queue_tail] = *command;
  }
  else
  {
    next = (uint8_t)((queue_head + 1U) % CMD_QUEUE_SIZE);
    if (next == queue_tail) { if (!primask) __enable_irq(); return 0U; }
    queue[queue_head] = *command;
    queue_head = next;
  }
  if (!primask) __enable_irq();
  return 1U;
}

static uint8_t dequeue(Command *command)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  if (queue_tail == queue_head) { if (!primask) __enable_irq(); return 0U; }
  *command = queue[queue_tail];
  queue_tail = (uint8_t)((queue_tail + 1U) % CMD_QUEUE_SIZE);
  if (!primask) __enable_irq();
  return 1U;
}

static void transmit_pending(uint32_t now_ms, uint8_t new_sequence)
{
  if (tx_busy) return;
  if (new_sequence) pending_seq = sequence++;
  tx_frame[0] = SOF0; tx_frame[1] = SOF1;
  tx_frame[2] = (uint8_t)(2U + pending.length);
  tx_frame[3] = pending.type; tx_frame[4] = pending_seq;
  memcpy(tx_frame + 5, pending.payload, pending.length);
  uint16_t crc = crc16(tx_frame + 2, (uint16_t)(3U + pending.length));
  tx_frame[5U + pending.length] = (uint8_t)crc;
  tx_frame[6U + pending.length] = (uint8_t)(crc >> 8);
  tx_busy = 1U;
  ++diagnostics.tx_frames;
  pending_sent_ms = now_ms;
  if (HAL_UART_Transmit_IT(port, tx_frame, (uint16_t)(7U + pending.length)) != HAL_OK)
    tx_busy = 0U;
}

void LDO_ProtocolInit(UART_HandleTypeDef *uart)
{
  port = uart;
  memset(&telemetry, 0, sizeof(telemetry));
  memset((void *)&diagnostics, 0, sizeof(diagnostics));
  restart_rx();
  Command info = { TYPE_GET_INFO, 0U, {0} };
  (void)queue_command(&info, 0U);
}

void LDO_ProtocolProcess(uint32_t now_ms)
{
  uint32_t age = now_ms - last_telemetry_ms;
  telemetry.telemetry_stale = (age > 100U) ? 1U : 0U;
  telemetry.connected = (last_telemetry_ms != 0U && age <= 500U) ? 1U : 0U;
  if (telemetry.connected && !link_was_connected)
  {
    Command output_off = { TYPE_SET_OUTPUT, 1U, {0U} };
    (void)queue_command(&output_off, 1U);
  }
  link_was_connected = telemetry.connected;

  if (pending_active)
  {
    if (pending_result != 0U)
    {
      pending_active = 0U;
      pending_result = 0U;
    }
    else if (!tx_busy && (now_ms - pending_sent_ms >= CMD_TIMEOUT_MS))
    {
      if (pending_retries < MAX_RETRIES)
      {
        ++pending_retries;
        transmit_pending(now_ms, 0U);
      }
      else
      {
        ++diagnostics.command_timeouts;
        pending_active = 0U;
      }
    }
  }
  if (!pending_active && !tx_busy && dequeue(&pending))
  {
    pending_active = 1U;
    diagnostics.pending_type = pending.type;
    pending_retries = 0U;
    pending_result = 0U;
    transmit_pending(now_ms, 1U);
  }
}

void LDO_SetLimits(uint32_t voltage_mv, uint32_t current_ma)
{
  if (voltage_mv > 27000U) voltage_mv = 27000U;
  if (current_ma > 5000U) current_ma = 5000U;
  Command command = { TYPE_SET_LIMITS, 8U, {0} };
  put32(command.payload, voltage_mv);
  put32(command.payload + 4, current_ma);
  (void)queue_command(&command, 0U);
}

void LDO_SetOutput(uint8_t enabled)
{
  Command command = { TYPE_SET_OUTPUT, 1U, { enabled ? 1U : 0U } };
  (void)queue_command(&command, enabled ? 0U : 1U);
}

uint8_t LDO_GetTelemetry(LDO_Telemetry *out)
{
  uint32_t before, after;
  unsigned spins;
  if (out == NULL) return 0U;
  for (spins = 0U; spins < 8U; ++spins)
  {
    before = telemetry_generation;
    if (before & 1U)
      continue;
    __DMB();
    *out = telemetry;
    __DMB();
    after = telemetry_generation;
    if (before == after)
      return out->connected;
  }
  *out = telemetry;
  return out->connected;
}

void LDO_GetDiagnostics(LDO_Diagnostics *out)
{
  if (out != NULL)
    *out = diagnostics;
}

void LDO_UartRxEvent(UART_HandleTypeDef *uart, uint16_t size)
{
  if (uart != port) return;
  diagnostics.rx_bytes += size;
  for (uint16_t i = 0; i < size; ++i) parse_byte(rx_chunk[i]);
  restart_rx();
}

void LDO_UartTxComplete(UART_HandleTypeDef *uart)
{
  if (uart == port) tx_busy = 0U;
}

void LDO_UartError(UART_HandleTypeDef *uart)
{
  if (uart != port) return;
  tx_busy = 0U;
  (void)HAL_UART_AbortReceive(port);
  restart_rx();
}
