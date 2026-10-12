#pragma once
#include <stdint.h>
typedef struct { int dummy; } I2C_HandleTypeDef;
typedef int HAL_StatusTypeDef;
typedef int GPIO_PinState;
#define HAL_OK 0
#define HAL_ERROR 1
#define GPIOG ((void*)1)
#define GPIO_PIN_15 0x8000
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define I2C_MEMADD_SIZE_8BIT 1
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef*,uint16_t,uint16_t,uint16_t,uint8_t*,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef*,uint16_t,uint16_t,uint16_t,uint8_t*,uint16_t,uint32_t);
void HAL_GPIO_WritePin(void*,uint16_t,GPIO_PinState);
void HAL_Delay(uint32_t);
