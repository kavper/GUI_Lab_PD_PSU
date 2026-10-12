#include "board_backlight.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint8_t regfile[256];
static int en, operations, fail_at, corrupt_at, enabled_before_config;
static int transfer(void) { return ++operations == fail_at ? HAL_ERROR : HAL_OK; }
static void check_current(void)
{
    if (en && (regfile[1] & 1)) {
        if (regfile[0x16] != 3 || regfile[0xA0] != 255 || regfile[0xA1] != 0x3F ||
            (regfile[0xA2] & 4) || (regfile[0xA5] & 0x80)) enabled_before_config = 1;
    }
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h,uint16_t addr,uint16_t r,uint16_t size,uint8_t *v,uint16_t count,uint32_t timeout)
{
    (void)h; assert(addr == 0x58 && size == 1 && count == 1 && timeout <= 50);
    if (transfer()) return HAL_ERROR;
    regfile[r] = *v;
    check_current();
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h,uint16_t addr,uint16_t r,uint16_t size,uint8_t *v,uint16_t count,uint32_t timeout)
{
    (void)h; assert(addr == 0x58 && size == 1 && count == 1 && timeout <= 50);
    if (transfer()) return HAL_ERROR;
    *v = regfile[r] ^ (operations == corrupt_at ? 1 : 0);
    return HAL_OK;
}
void HAL_GPIO_WritePin(void *port,uint16_t pin,GPIO_PinState state)
{
    assert(port == GPIOG && pin == GPIO_PIN_15);
    en = state;
}
void HAL_Delay(uint32_t t) { assert(t <= 10); }
static void reset(void)
{
    memset(regfile,0,sizeof(regfile));
    regfile[0x16] = 63;
    regfile[0xA1] = 0x7F; /* variant with 50mA factory current */
    regfile[0xA2] = 0x3C;
    regfile[0xA5] = 0x85;
    en = operations = fail_at = corrupt_at = enabled_before_config = 0;
}
int main(void)
{
    I2C_HandleTypeDef i2c = {0};
    reset();
    assert(BoardBacklight_Init40mA(&i2c));
    assert(en && regfile[0] == 255 && regfile[1] == 0x85);
    assert(regfile[0xA2] == 0x20 && regfile[0xA5] == 0x05); /* preserve unrelated settings */
    assert(!enabled_before_config);
    int total = operations;
    /* Each I2C operation can fail: every failure must shut down EN. */
    for (int i=1;i<=total;++i) {
        reset(); fail_at = i;
        assert(!BoardBacklight_Init40mA(&i2c));
        assert(!en && !enabled_before_config);
    }
    /* Every readback paired with a write must detect corrupt data. */
    const int readbacks[] = {2,4,6,8,10,14,16,18,20};
    for (unsigned i=0;i<sizeof(readbacks)/sizeof(readbacks[0]);++i) {
        reset(); corrupt_at = readbacks[i];
        assert(!BoardBacklight_Init40mA(&i2c));
        assert(!en);
    }
    puts("PASS: 40mA setup, sequencing, all I2C failures, register readback corruption");
}
