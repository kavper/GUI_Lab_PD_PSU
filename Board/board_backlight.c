/* LP8556: LED1 + LED2 in parallel, 20 mA per sink = 40 mA total.
 * Volatile configuration only: never burn EPROM. PWM is ignored in I2C mode.
 */
#include "board_backlight.h"
#define LP8556_ADDR (0x2CU << 1)
/* Last register, requested value, readback and HAL status for SWD diagnosis. */
volatile uint32_t BoardBacklightDiagnostics[4];
volatile uint32_t BoardBacklightReady;
static int write_verify(I2C_HandleTypeDef *h, uint8_t reg, uint8_t val)
{
    uint8_t readback = 0;
    BoardBacklightDiagnostics[0] = reg;
    BoardBacklightDiagnostics[1] = val;
    BoardBacklightDiagnostics[2] = 0x100;
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(h, LP8556_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 50);
    if (status == HAL_OK)
        status = HAL_I2C_Mem_Read(h, LP8556_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &readback, 1, 50);
    BoardBacklightDiagnostics[2] = readback;
    BoardBacklightDiagnostics[3] = status;
    return status == HAL_OK && readback == val;
}
int BoardBacklight_Init40mA(I2C_HandleTypeDef *h)
{
    BoardBacklightReady = 0;
    /* EN powers the control logic. Brightness=0 and BL_CTL=0 prevent LED current
     * until all configuration writes have been read back. FAST avoids EPROM reload. */
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_SET);
    HAL_Delay(10);
    if (!write_verify(h, 0x00, 0x00) || !write_verify(h, 0x01, 0x84) ||
        !write_verify(h, 0x16, 0x03) || !write_verify(h, 0xA0, 0xFF) ||
        !write_verify(h, 0xA1, 0x3F)) goto fail;
    uint8_t cfg2, cfg5;
    if (HAL_I2C_Mem_Read(h, LP8556_ADDR, 0xA2, 1, &cfg2, 1, 50) != HAL_OK ||
        HAL_I2C_Mem_Read(h, LP8556_ADDR, 0xA5, 1, &cfg5, 1, 50) != HAL_OK) goto fail;
    /* 5V supply: clear 5.2V UVLO threshold, external ISET and PWM-only BL_ON; clear PWM_DIRECT. */
    if (!write_verify(h, 0xA2, cfg2 & ~0x1C) || !write_verify(h, 0xA5, cfg5 & ~0x80) ||
        !write_verify(h, 0x00, 0xFF) || !write_verify(h, 0x01, 0x85)) goto fail;
    BoardBacklightReady = 1;
    return 1;
fail:
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_RESET);
    return 0;
}
