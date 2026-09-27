#include "g0_frame.h"
#include <string.h>

uint16_t g0_crc16_ccitt(const uint8_t *data, uint16_t size)
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
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int g0_decode_telemetry68(const uint8_t *payload, G0RawTelemetry *out)
{
  uint8_t i;
  if (payload == 0 || out == 0)
    return -1;
  if (payload[0] != 1U || payload[1] != 1U)
    return -1;
  memset(out, 0, sizeof(*out));
  out->protocol_version = payload[0];
  out->telemetry_version = payload[1];
  out->status_flags = le16(payload + 2);
  out->fault_flags = le32(payload + 4);
  out->uptime_ms = le32(payload + 8);
  out->vin_mv = le32(payload + 12);
  out->vout_mv = le32(payload + 16);
  out->iout_ua = (int32_t)le32(payload + 20);
  out->iout_adc_raw = (int32_t)le32(payload + 24);
  out->dac_cv_readback_mv = le32(payload + 28);
  out->dac_cc_readback_mv = le32(payload + 32);
  out->requested_voltage_mv = le32(payload + 36);
  out->requested_current_ma = le32(payload + 40);
  out->applied_voltage_mv = le32(payload + 44);
  out->applied_current_ma = le32(payload + 48);
  out->preregulator_mv = le32(payload + 52);
  for (i = 0; i < 4U; ++i)
    out->temperature_centi_c[i] = (int16_t)le16(payload + 56U + (uint16_t)(i * 2U));
  out->mode = payload[64];
  out->startup = payload[65];
  out->reserved = le16(payload + 66);
  return 0;
}

void g0_parser_init(G0FrameParser *parser)
{
  if (parser == 0)
    return;
  memset(parser, 0, sizeof(*parser));
}

int g0_parser_push(G0FrameParser *parser, uint8_t value,
                   uint8_t *type, uint8_t *seq,
                   uint8_t *payload_out, uint8_t *payload_len)
{
  if (parser == 0)
    return 0;
  if (parser->pos == 0U)
  {
    if (value == 0xA5U)
      parser->frame[parser->pos++] = value;
    return 0;
  }
  if (parser->pos == 1U)
  {
    if (value == 0x5AU)
      parser->frame[parser->pos++] = value;
    else
      parser->pos = (value == 0xA5U) ? 1U : 0U;
    return 0;
  }
  parser->frame[parser->pos++] = value;
  if (parser->pos == 3U)
  {
    if (value < 2U || value > (2U + 80U))
    {
      parser->pos = 0U;
      return 0;
    }
    parser->total = (uint8_t)(2U + 1U + value + 2U);
  }
  if (parser->total != 0U && parser->pos == parser->total)
  {
    const uint16_t received = le16(parser->frame + parser->total - 2U);
    const uint16_t calculated = g0_crc16_ccitt(parser->frame + 2, (uint16_t)(parser->frame[2] + 1U));
    const uint8_t plen = (uint8_t)(parser->frame[2] - 2U);
    int ok = 0;
    if (received == calculated)
    {
      ++parser->valid_frames;
      if (type) *type = parser->frame[3];
      if (seq) *seq = parser->frame[4];
      if (payload_len) *payload_len = plen;
      if (payload_out)
        memcpy(payload_out, parser->frame + 5, plen);
      ok = 1;
    }
    else
      ++parser->crc_errors;
    parser->pos = 0U;
    parser->total = 0U;
    return ok;
  }
  return 0;
}
