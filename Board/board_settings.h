#pragma once
#include "stm32h7rsxx_hal.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t available, loaded, pending, commits, errors, generation, failed; } BoardSettingsDiagnostics;
extern volatile BoardSettingsDiagnostics BoardSettings_diag;
void BoardSettings_Init(I2C_HandleTypeDef *bus);
/* Only the GUI task calls this: same I2C bus owner as the GT911 touch controller. */
void BoardSettings_Poll(uint32_t now);
#ifdef __cplusplus
}
#endif
