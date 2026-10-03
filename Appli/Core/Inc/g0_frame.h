#ifndef G0_FRAME_H
#define G0_FRAME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Binary map used by Appli/Core/Src/ldo_protocol.c (68-byte telemetry).
   Offset 20 is signed current in microamps. Offset 24 is raw ADC and is
   never a current. status_flags is little-endian at offset 2. */

#define G0_TELEMETRY_BYTES 68u
#define G0_CURRENT_VALID_BIT 4u

typedef struct
{
  uint8_t protocol_version;
  uint8_t telemetry_version;
  uint16_t status_flags;
  uint32_t fault_flags;
  uint32_t uptime_ms;
  uint32_t vin_mv;
  uint32_t vout_mv;
  int32_t iout_ua;
  int32_t iout_adc_raw;
  uint32_t dac_cv_readback_mv;
  uint32_t dac_cc_readback_mv;
  uint32_t requested_voltage_mv;
  uint32_t requested_current_ma;
  uint32_t applied_voltage_mv;
  uint32_t applied_current_ma;
  uint32_t preregulator_mv;
  int16_t temperature_centi_c[4]; /* Legacy name; wire unit is 0.1 C. */
  uint8_t mode;
  uint8_t startup;
  uint16_t reserved;
} G0RawTelemetry;

typedef struct
{
  uint8_t frame[87];
  uint8_t pos;
  uint8_t total;
  uint32_t valid_frames;
  uint32_t crc_errors;
} G0FrameParser;

uint16_t g0_crc16_ccitt(const uint8_t *data, uint16_t size);
int g0_decode_telemetry68(const uint8_t *payload, G0RawTelemetry *out);
void g0_parser_init(G0FrameParser *parser);
/* Returns 1 when a CRC-valid frame is complete. Payload is copied to payload_out. */
int g0_parser_push(G0FrameParser *parser, uint8_t value,
                   uint8_t *type, uint8_t *seq,
                   uint8_t *payload_out, uint8_t *payload_len);

#ifdef __cplusplus
}
#endif

#endif
