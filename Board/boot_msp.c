/* Shared generated MSP remains unmodified. XSPI2 is powered at 3.3 V. */
#define HAL_MspInit DK_HAL_MspInit
#include "../Boot/Core/Src/stm32h7rsxx_hal_msp.c"
#undef HAL_MspInit
void HAL_MspInit(void)
{
    if (HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY) != HAL_OK) Error_Handler();
    __HAL_RCC_SBS_CLK_ENABLE();
    HAL_PWREx_EnableXSPIM1();
    HAL_PWREx_EnableXSPIM2();
    HAL_SBS_EnableIOSpeedOptimize(SBS_IO_XSPI1_HSLV);
    HAL_SBS_DisableIOSpeedOptimize(SBS_IO_XSPI2_HSLV);
}
