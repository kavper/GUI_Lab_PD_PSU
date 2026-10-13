/* IS25LX256 (32 MiB, 3.3 V), APS256XXN-OB9 (32 MiB, 1.8 V).
 * Flash remains at 100 MHz; PSRAM configures at 100 MHz then runs at 200 MHz.
 * Flash clock MUST be PN6/CLK.
 * APS256 LC=5 includes the final address clock: XSPI dummy count is LC-1.
 */
#include "board_memory.h"
#include "extmem_manager.h"
#include <string.h>

extern XSPI_HandleTypeDef hxspi1, hxspi2;
volatile uint32_t BoardRamDiagnostics[4];

static int psram_register(uint32_t address, uint8_t *value, int write, uint32_t dummy)
{
    XSPI_RegularCmdTypeDef cmd = {0};
    uint8_t data[2] = {*value, *value};
    cmd.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    cmd.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    cmd.Instruction = write ? 0xC0 : 0x40;
    cmd.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    cmd.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    cmd.Address = address;
    cmd.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    cmd.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    cmd.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    cmd.DataMode = HAL_XSPI_DATA_8_LINES;
    cmd.DataDTRMode = HAL_XSPI_DATA_DTR_ENABLE;
    cmd.DataLength = 2;
    cmd.DummyCycles = write ? 0 : dummy;
    cmd.DQSMode = write ? HAL_XSPI_DQS_DISABLE : HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(&hxspi1, &cmd, 100) != HAL_OK) return 0;
    if (write) return HAL_XSPI_Transmit(&hxspi1, data, 100) == HAL_OK;
    if (HAL_XSPI_Receive(&hxspi1, data, 100) != HAL_OK) return 0;
    *value = data[0];
    return 1;
}

static int psram_fast_config(EXTMEM_DRIVER_PSRAM_ObjectTypeDef *p)
{
    uint8_t mr0 = 0, mr4 = 0, verify = 0;
    if (!psram_register(0, &mr0, 0, 4) || !psram_register(4, &mr4, 0, 4)) return 0;
    // AP Memory rev 1.2: variable read LC=7, write WLC=7 up to 200 MHz.
    mr4 = (mr4 & 0x1F) | 0x20;
    if (!psram_register(4, &mr4, 1, 0)) return 0;
    mr0 = (mr0 & 0x03) | 0x10;
    if (!psram_register(0, &mr0, 1, 0)) return 0;
    if (!psram_register(0, &verify, 0, 6) || verify != mr0) return 0;
    BoardRamDiagnostics[1] = verify;
    if (!psram_register(4, &verify, 0, 6) || verify != mr4) return 0;
    BoardRamDiagnostics[2] = verify;
    p->psram_public.REG_DummyCycle = 6;
    p->psram_public.Read_DummyCycle = 6;
    p->psram_public.Write_DummyCycle = 6;
    p->psram_public.FreqMax = 200000000;
    hxspi1.Init.Refresh = 600; // 3 us at 200 MHz, below tCEM=4 us.
    hxspi1.Instance->DCR4 = 600;
    if (HAL_XSPI_SetClockPrescaler(&hxspi1, 0) != HAL_OK) return 0;
    if (!psram_register(0, &verify, 0, 6) || verify != mr0) return 0;
    BoardRamDiagnostics[3] = 200000000;
    return 1;
}

static int psram_x16_config(void)
{
    XSPI_RegularCmdTypeDef cmd = {0};
    uint8_t data[2] = {0x44, 0x44};
    cmd.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    cmd.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    cmd.Instruction = 0xC0;
    cmd.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    cmd.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    cmd.Address = 8;
    cmd.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    cmd.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    cmd.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    cmd.DataMode = HAL_XSPI_DATA_8_LINES;
    cmd.DataDTRMode = HAL_XSPI_DATA_DTR_ENABLE;
    cmd.DataLength = 2;
    /* LC=1 includes the final address clock; no extra dummy clock for MR writes. */
    cmd.DummyCycles = 0;
    if (HAL_XSPI_Command(&hxspi1, &cmd, 100) != HAL_OK ||
        HAL_XSPI_Transmit(&hxspi1, data, 100) != HAL_OK) return 0;
    cmd.Instruction = 0x40;
    cmd.DummyCycles = 4;
    cmd.DQSMode = HAL_XSPI_DQS_ENABLE;
    data[0] = data[1] = 0;
    if (HAL_XSPI_Command(&hxspi1, &cmd, 100) != HAL_OK ||
        HAL_XSPI_Receive(&hxspi1, data, 100) != HAL_OK) return 0;
    BoardRamDiagnostics[0] = data[0] | ((uint32_t)data[1] << 8);
    return (data[0] & 0xC7) == 0x44;
}

static int link_init(XSPI_HandleTypeDef *h, XSPI_TypeDef *instance, uint32_t type, uint32_t port)
{
    XSPIM_CfgTypeDef manager = {0};
    memset(h, 0, sizeof(*h));
    h->Instance = instance;
    h->Init.FifoThresholdByte = 4;
    h->Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
    h->Init.MemoryType = type;
    h->Init.MemorySize = HAL_XSPI_SIZE_256MB; /* HAL enum is in Mbits */
    // tCPH at 200 MHz is 24 ns: 5 cycles provide 25 ns.
    h->Init.ChipSelectHighTimeCycle = instance == XSPI1 ? 5 : 3;
    h->Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
    h->Init.ClockPrescaler = 1;
    h->Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
    h->Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
    h->Init.WrapSize = instance == XSPI1 ? HAL_XSPI_WRAP_32_BYTES : HAL_XSPI_WRAP_NOT_SUPPORTED;
    /* 2 KiB physical RAM rows; bound every transaction, including DMA bursts.
     * 300 clock cycles at 100 MHz = 3 us, below the 4 us tCEM limit. */
    h->Init.ChipSelectBoundary = instance == XSPI1 ? HAL_XSPI_BONDARYOF_2KB : HAL_XSPI_BONDARYOF_NONE;
    h->Init.Refresh = instance == XSPI1 ? 300 : 0;
    if (HAL_XSPI_Init(h) != HAL_OK) return 0;
    manager.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
    manager.IOPort = port;
    return HAL_XSPIM_Config(h, &manager, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK;
}

int BoardMemory_Init(int with_psram)
{
    memset(extmem_list_config, 0, sizeof(extmem_list_config));
    if (!link_init(&hxspi2, XSPI2, HAL_XSPI_MEMTYPE_MICRON, HAL_XSPIM_IOPORT_2)) return 0;
    extmem_list_config[0].MemType = EXTMEM_NOR_SFDP;
    extmem_list_config[0].Handle = &hxspi2;
    extmem_list_config[0].ConfigType = EXTMEM_LINK_CONFIG_8LINES;
    extmem_list_config[0].NorSfdpObject.sfdp_public.MaxFreq = 100000000;
    if (EXTMEM_Init(EXTMEMORY_1, HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_XSPI2)) != EXTMEM_OK) return 0;
    if (extmem_list_config[0].NorSfdpObject.sfdp_private.ManuID != EXTMEM_MANUFACTURER_ISSI ||
        extmem_list_config[0].NorSfdpObject.sfdp_private.FlashSize != 25) return 0;
    if (!with_psram) return 1;
    if (!link_init(&hxspi1, XSPI1, HAL_XSPI_MEMTYPE_APMEM_16BITS, HAL_XSPIM_IOPORT_1)) return 0;
    /* Global Reset: instruction FF on DQ0..7, four clock periods under CE#.
     * Required on both cold power-up and MCU-only reset (MR8 otherwise persists). */
    HAL_Delay(1);
    XSPI_RegularCmdTypeDef reset = {0};
    reset.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    reset.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    reset.Instruction = 0xFF;
    reset.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    reset.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    reset.AddressMode = HAL_XSPI_ADDRESS_NONE;
    reset.DataMode = HAL_XSPI_DATA_NONE;
    reset.DummyCycles = 3;
    if (HAL_XSPI_Command(&hxspi1, &reset, 100) != HAL_OK) return 0;
    HAL_Delay(1);
    extmem_list_config[1].MemType = EXTMEM_PSRAM;
    extmem_list_config[1].Handle = &hxspi1;
    extmem_list_config[1].ConfigType = EXTMEM_LINK_CONFIG_16LINES;
    EXTMEM_DRIVER_PSRAM_ObjectTypeDef *p = &extmem_list_config[1].PsramObject;
    p->psram_public.MemorySize = HAL_XSPI_SIZE_256MB;
    p->psram_public.FreqMax = 100000000;
    p->psram_public.NumberOfConfig = 0; /* MR write handled with latency/readback below */
    p->psram_public.config[0].WriteMask = 0x47;
    p->psram_public.config[0].WriteValue = 0x44;
    p->psram_public.config[0].REGAddress = 8; /* MR8: X16, hybrid 16-word = 32-byte wrap */
    p->psram_public.ReadREG = 0x40;
    p->psram_public.WriteREG = 0xC0;
    p->psram_public.ReadREGSize = 2;
    p->psram_public.REG_DummyCycle = 4;
    p->psram_public.Write_command = 0xA0;
    p->psram_public.Write_DummyCycle = 4;
    p->psram_public.Read_command = 0x20;
    p->psram_public.WrapRead_command = 0x00;
    p->psram_public.Read_DummyCycle = 4;
    return EXTMEM_Init(EXTMEMORY_2, HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_XSPI1)) == EXTMEM_OK &&
           psram_x16_config() && psram_fast_config(p);
}

void BoardQuietDisplay(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5 | GPIO_PIN_15, GPIO_PIN_RESET);
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Pin = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &g);
    g.Pin = GPIO_PIN_5 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &g);
}
