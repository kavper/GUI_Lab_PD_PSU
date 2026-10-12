/* Reuse ST loader ABI; propagate board initialization failures from Init. */
#define Init ST_Init
#include "../../Middlewares/ST/STM32_ExtMem_Loader/STM32Cube/stm32_loader_api.c"
#undef Init
#include "board_memory.h"
extern void SystemClock_Config(void);
uint32_t extmemloader_Init(void)
{
    SystemInit();
    __disable_irq();
    HAL_MPU_Disable();
    if (SCB->CCR & SCB_CCR_DC_Msk) SCB_DisableDCache();
    if (SCB->CCR & SCB_CCR_IC_Msk) SCB_DisableICache();
    /* Init does not enter through Reset_Handler: restore mutable HAL defaults. */
    SystemCoreClock = 64000000;
    uwTickFreq = HAL_TICK_FREQ_DEFAULT;
    if (HAL_Init() != HAL_OK) return 1;
    BoardQuietDisplay();
    SystemClock_Config();
    return BoardMemory_Init(0) ? 0 : 1;
}
__attribute__((used)) uint32_t Init(void)
{
    memset(&__bss_start__, 0, (uintptr_t)&__bss_end__ - (uintptr_t)&__bss_start__);
    if (extmemloader_Init() != 0) return 0;
    if (memory_mapmode(MEM_MAPENABLE) != MEM_OK) return 0;
    MemoryMappedMode = MEM_MAPENABLE;
    return 1;
}
