#include "psu_format.h"
#include <stdio.h>

void psu_format_voltage(char *dst, size_t n, uint32_t voltage_mv)
{
  if (dst == 0 || n == 0U)
    return;
  voltage_mv = voltage_mv > 999999U ? 999999U : voltage_mv;
  if (voltage_mv < 10000U)
    (void)snprintf(dst, n, "%u.%03u V", (unsigned)(voltage_mv / 1000U), (unsigned)(voltage_mv % 1000U));
  else
    (void)snprintf(dst, n, "%u.%02u V", (unsigned)(voltage_mv / 1000U), (unsigned)((voltage_mv % 1000U) / 10U));
}

void psu_format_current_ma(char *dst, size_t n, uint32_t current_ma)
{
  if (dst == 0 || n == 0U)
    return;
  (void)snprintf(dst, n, "%u.%03u A", (unsigned)(current_ma / 1000U), (unsigned)(current_ma % 1000U));
}

void psu_format_current_ua(char *dst, size_t n, int32_t current_ua)
{
  int32_t abs_ua;
  int32_t ma;
  if (dst == 0 || n == 0U)
    return;
  abs_ua = current_ua < 0 ? -current_ua : current_ua;
  ma = (abs_ua + 500) / 1000;
  if (current_ua < 0)
    (void)snprintf(dst, n, "-%d.%03d A", (int)(ma / 1000), (int)(ma % 1000));
  else
    (void)snprintf(dst, n, "%d.%03d A", (int)(ma / 1000), (int)(ma % 1000));
}

void psu_format_power_mw(char *dst, size_t n, uint32_t power_mw)
{
  if (dst == 0 || n == 0U)
    return;
  (void)snprintf(dst, n, "%u.%02u W", (unsigned)(power_mw / 1000U), (unsigned)((power_mw % 1000U) / 10U));
}

void psu_format_deci_c(char *dst, size_t n, int16_t deci_c)
{
  int32_t abs_v;
  if (dst == 0 || n == 0U)
    return;
  abs_v = deci_c < 0 ? -(int32_t)deci_c : (int32_t)deci_c;
  if (deci_c < 0)
    (void)snprintf(dst, n, "-%d.%01d C", (int)(abs_v / 10), (int)(abs_v % 10));
  else
    (void)snprintf(dst, n, "%d.%01d C", (int)(abs_v / 10), (int)(abs_v % 10));
}

