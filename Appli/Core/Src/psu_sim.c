#include "psu_app.h"
#include "g0_frame.h"

#if defined(SIMULATOR) || defined(PSU_SIMULATOR)

#include <string.h>

/* Simulator-only measurements. The target build compiles this file to nothing. */
void psu_sim_tick(uint32_t now_ms)
{
  static uint32_t phase;
  PsuG0Sample sample;
  uint8_t frame_payload[68];
  G0RawTelemetry raw;
  memset(&sample, 0, sizeof(sample));
  memset(frame_payload, 0, sizeof(frame_payload));
  frame_payload[0] = 1U;
  frame_payload[1] = 1U;
  frame_payload[2] = 0x11U; /* status: output bit and current-valid bit */
  frame_payload[16] = 0x10;
  frame_payload[17] = 0x2E; /* 12048 mV little endian */
  frame_payload[20] = 0x40;
  frame_payload[21] = 0xE2;
  frame_payload[22] = 0x12; /* 1_236_032 uA */
  if ((phase % 40U) == 0U)
  {
    /* Inject one raw line. It must stay unparsed: no TC grammar is in the repo. */
    const char *line = "TC raw-unparsed\n";
    g4_rx_bytes(psu_g4(), (const uint8_t *)line, 16U, now_ms);
  }
  if (g0_decode_telemetry68(frame_payload, &raw) == 0)
  {
    PsuSnapshot live;
    psu_snapshot(&live);
    sample.connected = (phase % 80U) < 70U;
    sample.current_valid = sample.connected;
    sample.current_calibrated = 1U;
    sample.status_flags = (uint16_t)(0x0010U | (live.output_requested ? 0x0001U : 0U));
    sample.vout_mv = raw.vout_mv;
    sample.vin_mv = 20000U;
    sample.iout_ua = raw.iout_ua;
    sample.iout_adc_raw = raw.iout_adc_raw;
    sample.applied_voltage_mv = live.requested_mv;
    sample.applied_current_ma = live.requested_ma;
    sample.temp_centi_c[0] = 386;
    sample.temp_centi_c[3] = 412;
    sample.mode = 1U;
    sample.maximum_voltage_mv = 27000U;
    sample.maximum_current_ma = 5000U;
    sample.protocol_version = 1U;
    sample.telemetry_version = 1U;
    psu_app_observe_g0(&sample, now_ms);
  }
  ++phase;
  (void)now_ms;
}

#endif
