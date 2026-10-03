#ifndef LDO_PROTOCOL_H
#define LDO_PROTOCOL_H

#include "stm32h7rsxx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
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
  uint8_t protocol_version;
  uint8_t telemetry_version;
  uint8_t current_calibrated;
  uint8_t current_valid;
  uint8_t telemetry_stale;
  uint8_t connected;
  uint16_t telemetry_period_ms;
  uint16_t reserved;
  uint32_t maximum_voltage_mv;
  uint32_t maximum_current_ma;
  uint32_t capability_flags;
} LDO_Telemetry;

typedef struct
{
  uint32_t rx_bytes;
  uint32_t valid_frames;
  uint32_t crc_errors;
  uint32_t tx_frames;
  uint32_t ack_frames;
  uint32_t nack_frames;
  uint32_t command_timeouts;
  uint32_t uart_errors;
  uint32_t last_uart_error;
  uint32_t rx_start_failures;
  uint32_t rx_overflows;
  uint32_t tx_start_failures;
  uint8_t rx_active;
  uint8_t last_nack_reason;
  uint8_t pending_type;
} LDO_Diagnostics;

void LDO_ProtocolInit(UART_HandleTypeDef *uart);
void LDO_ProtocolProcess(uint32_t now_ms);
void LDO_SetLimits(uint32_t voltage_mv, uint32_t current_ma);
void LDO_SetOutput(uint8_t enabled);
uint8_t LDO_GetTelemetry(LDO_Telemetry *out);
void LDO_GetDiagnostics(LDO_Diagnostics *out);
void LDO_UartRxEvent(UART_HandleTypeDef *uart, uint16_t size);
void LDO_UartTxComplete(UART_HandleTypeDef *uart);
void LDO_UartError(UART_HandleTypeDef *uart);

#ifdef __cplusplus
}
#endif

#endif
