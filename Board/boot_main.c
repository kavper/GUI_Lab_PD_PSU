/* Custom board startup. CubeMX sources are included read-only for shared helpers. */
#define main DK_BootMain
#define SystemClock_Config DK_SystemClock_Config
#include "../Boot/Core/Src/main.c"
#undef main
#undef SystemClock_Config
#include "board_memory.h"
BOOTStatus_TypeDef MapMemory(void);
BOOTStatus_TypeDef JumpToApplication(void);

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL1.PLLM = 4;
  RCC_OscInitStruct.PLL1.PLLN = 100;
  RCC_OscInitStruct.PLL1.PLLP = 1;
  RCC_OscInitStruct.PLL1.PLLQ = 2;
  RCC_OscInitStruct.PLL1.PLLR = 2;
  RCC_OscInitStruct.PLL1.PLLS = 2;
  RCC_OscInitStruct.PLL1.PLLT = 2;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL2.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL2.PLLM = 6;
  RCC_OscInitStruct.PLL2.PLLN = 100;
  RCC_OscInitStruct.PLL2.PLLP = 2;
  RCC_OscInitStruct.PLL2.PLLQ = 2;
  RCC_OscInitStruct.PLL2.PLLR = 2;
  RCC_OscInitStruct.PLL2.PLLS = 2;
  RCC_OscInitStruct.PLL2.PLLT = 2;
  RCC_OscInitStruct.PLL2.PLLFractional = 0;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL3.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL3.PLLM = 2;
  RCC_OscInitStruct.PLL3.PLLN = 50;
  RCC_OscInitStruct.PLL3.PLLP = 2;
  RCC_OscInitStruct.PLL3.PLLQ = 2;
  RCC_OscInitStruct.PLL3.PLLR = 24;
  RCC_OscInitStruct.PLL3.PLLS = 2;
  RCC_OscInitStruct.PLL3.PLLT = 2;
  RCC_OscInitStruct.PLL3.PLLFractional = 0;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK4|RCC_CLOCKTYPE_PCLK5;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

#ifndef BOARD_EXTERNAL_LOADER
int main(void)
{
    HAL_Init();
    BoardQuietDisplay();
    SystemClock_Config();
    MX_HPDMA1_Init();
    MX_SBS_Init();
    if (!BoardMemory_Init(1)) Error_Handler();
    if (MapMemory() != BOOT_OK) Error_Handler();
    volatile uint32_t *ram = (volatile uint32_t *)0x90000000;
    /* Address-line alias detection over the full 32 MiB and data-line patterns. */
    for (unsigned bit = 0; bit < 23; ++bit) ram[1U << bit] = 0xA5000000U | bit;
    ram[0] = 0x12345678;
    __DSB();
    if (ram[0] != 0x12345678) Error_Handler();
    for (unsigned bit = 0; bit < 23; ++bit)
        if (ram[1U << bit] != (0xA5000000U | bit)) Error_Handler();
    for (unsigned bit = 0; bit < 32; ++bit) {
        ram[0] = 1U << bit;
        __DSB();
        if (ram[0] != (1U << bit)) Error_Handler();
        ram[0] = ~(1U << bit);
        __DSB();
        if (ram[0] != ~(1U << bit)) Error_Handler();
    }
    /* Cross a physical 2 KiB row and exercise the last cache line. */
    for (unsigned i = 0; i < 32; ++i) {
        ram[496 + i] = 0x5A000000U | i;
        ram[0x7FFFE0 + i] = 0xC3000000U | i;
    }
    __DSB();
    for (unsigned i = 0; i < 32; ++i)
        if (ram[496 + i] != (0x5A000000U | i) ||
            ram[0x7FFFE0 + i] != (0xC3000000U | i)) Error_Handler();
    const volatile uint32_t *vectors = (const volatile uint32_t *)0x70000000;
    if (vectors[0] < 0x20000000 || vectors[0] > 0x20010000 || (vectors[0] & 7) ||
        vectors[1] < 0x70000001 || vectors[1] >= 0x70200000 || !(vectors[1] & 1)) Error_Handler();
    if (JumpToApplication() != BOOT_OK) Error_Handler();
    for (;;) {}
}

#endif
