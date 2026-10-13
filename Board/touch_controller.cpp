/* GT911 polling: bounded I2C transfers, status acknowledged after coordinates. */
#include <STM32TouchController.hpp>
#include "main.h"
#include <touchgfx/hal/HAL.hpp>
extern "C" I2C_HandleTypeDef hi2c1;
void STM32TouchController::init() {}
bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    static bool down = false;
    static int32_t lastX = 0, lastY = 0;
    uint8_t status, xy[4], zero = 0;
    if (HAL_I2C_Mem_Read(&hi2c1, 0xBA, 0x814E, I2C_MEMADD_SIZE_16BIT, &status, 1, 10) != HAL_OK)
        return down = false;
    if (status & 0x80) {
        uint8_t count = status & 0x0F;
        down = false;
        if (count > 0 && count <= 5 &&
            HAL_I2C_Mem_Read(&hi2c1, 0xBA, 0x8150, I2C_MEMADD_SIZE_16BIT, xy, 4, 10) == HAL_OK) {
            lastX = xy[0] | (xy[1] << 8);
            lastY = xy[2] | (xy[3] << 8);
            down = lastX < touchgfx::HAL::DISPLAY_WIDTH && lastY < touchgfx::HAL::DISPLAY_HEIGHT;
        }
        if (HAL_I2C_Mem_Write(&hi2c1, 0xBA, 0x814E, I2C_MEMADD_SIZE_16BIT, &zero, 1, 10) != HAL_OK)
            down = false;
    }
    // Physical panel is mounted upside down; GUI coordinates stay 800x480.
    x = touchgfx::HAL::DISPLAY_WIDTH - 1 - lastX;
    y = touchgfx::HAL::DISPLAY_HEIGHT - 1 - lastY;
    return down;
}
