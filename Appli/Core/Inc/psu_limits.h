#ifndef PSU_LIMITS_H
#define PSU_LIMITS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PSU_VOLTAGE_MAX_MV 27000u
#define PSU_CURRENT_MAX_MA 5000u
#define PSU_POWER_ABS_MAX_MW 135000u

uint32_t psu_clamp_voltage_mv(uint32_t voltage_mv);
uint32_t psu_clamp_current_ma(uint32_t current_ma);
uint32_t psu_clamp_voltage_to_hw(uint32_t voltage_mv, uint32_t hw_max_mv);
uint32_t psu_clamp_current_to_hw(uint32_t current_ma, uint32_t hw_max_ma);
/* Vout(mV) * max(Iout(uA), 0) / 1e6 = mW. Saturates instead of wrapping. */
uint32_t psu_output_power_mw(uint32_t vout_mv, int32_t iout_ua);
int32_t psu_display_current_ua(int32_t iout_ua, int current_valid);

#ifdef __cplusplus
}
#endif

#endif
