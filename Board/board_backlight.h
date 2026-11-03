#pragma once
#include "stm32h7rsxx_hal.h"
#ifdef __cplusplus
extern "C" {
#endif
int BoardBacklight_Init60mA(I2C_HandleTypeDef *i2c);
/* Brightness via LP8556 BRIGHTNESS register (0x00, I2C mode). BL_EN only enables
 * the driver. GUI task only: I2C1 is shared with GT911 touch and the EEPROM. */
uint8_t BoardBacklight_LevelForPercent(uint8_t percent);
int BoardBacklight_SetPercent(I2C_HandleTypeDef *i2c, uint8_t percent);
#ifdef __cplusplus
}
#endif
