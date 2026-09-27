#include "psu_limits.h"

uint32_t psu_clamp_voltage_mv(uint32_t voltage_mv)
{
  return voltage_mv > PSU_VOLTAGE_MAX_MV ? PSU_VOLTAGE_MAX_MV : voltage_mv;
}

uint32_t psu_clamp_current_ma(uint32_t current_ma)
{
  return current_ma > PSU_CURRENT_MAX_MA ? PSU_CURRENT_MAX_MA : current_ma;
}

uint32_t psu_clamp_voltage_to_hw(uint32_t voltage_mv, uint32_t hw_max_mv)
{
  uint32_t capped = psu_clamp_voltage_mv(voltage_mv);
  if (hw_max_mv != 0U && capped > hw_max_mv)
    capped = hw_max_mv;
  return capped;
}

uint32_t psu_clamp_current_to_hw(uint32_t current_ma, uint32_t hw_max_ma)
{
  uint32_t capped = psu_clamp_current_ma(current_ma);
  if (hw_max_ma != 0U && capped > hw_max_ma)
    capped = hw_max_ma;
  return capped;
}

uint32_t psu_output_power_mw(uint32_t vout_mv, int32_t iout_ua)
{
  uint64_t product;
  if (iout_ua <= 0)
    return 0U;
  product = (uint64_t)vout_mv * (uint64_t)iout_ua;
  product /= 1000000ULL;
  if (product > 0xFFFFFFFFULL)
    return 0xFFFFFFFFU;
  return (uint32_t)product;
}

int32_t psu_display_current_ua(int32_t iout_ua, int current_valid)
{
  if (!current_valid || iout_ua < 0)
    return 0;
  return iout_ua;
}
