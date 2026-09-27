#ifndef PSU_FORMAT_H
#define PSU_FORMAT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Below 10 V: 3 decimals. From 10 V: 2 decimals. Always a dot, never a comma. */
void psu_format_voltage(char *dst, size_t n, uint32_t voltage_mv);
void psu_format_current_ma(char *dst, size_t n, uint32_t current_ma);
void psu_format_current_ua(char *dst, size_t n, int32_t current_ua);
void psu_format_power_mw(char *dst, size_t n, uint32_t power_mw);
void psu_format_centi_c(char *dst, size_t n, int16_t centi_c);

#ifdef __cplusplus
}
#endif

#endif
