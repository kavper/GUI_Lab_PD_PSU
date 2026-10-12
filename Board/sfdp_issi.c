/* Keep ST's SFDP parser intact; handle IS25LX256 without an SCCR map.
 * ISSI Rev. A6 tables 6.6/8.1: VCR[1]=1F selects 16 dummy cycles for
 * Octal DDR fast reads. Check this in SPI before entering DDR (odd register
 * addresses cannot be used in DDR). No nonvolatile register is written.
 */
#define SFDP_BuildGenericDriver STM32_SFDP_BuildGenericDriver
#include "../Middlewares/ST/STM32_ExtMem_Manager/nor_sfdp/stm32_sfdp_data.c"
#undef SFDP_BuildGenericDriver
volatile uint32_t BoardFlashDiagnostics[4];

SFDP_StatusTypeDef SFDP_BuildGenericDriver(EXTMEM_DRIVER_NOR_SFDP_ObjectTypeDef *Object,
                                         uint8_t *FreqUpdated)
{
    uint8_t vcr = 0xFF;
    HAL_StatusTypeDef read_status = HAL_ERROR;
    if (Object->sfdp_private.ManuID == EXTMEM_MANUFACTURER_ISSI &&
        Object->sfdp_private.DriverInfo.SpiPhyLink == PHY_LINK_1S1S1S)
        read_status = SAL_XSPI_CommandSendReadAddress(&Object->sfdp_private.SALObject,
                                    0x85, 1, &vcr, 1, EXTMEM_MANUFACTURER_ISSI);
    SFDP_StatusTypeDef status = STM32_SFDP_BuildGenericDriver(Object, FreqUpdated);
    BoardFlashDiagnostics[0] = status;
    BoardFlashDiagnostics[1] = read_status;
    BoardFlashDiagnostics[2] = vcr;
    BoardFlashDiagnostics[3] = Object->sfdp_private.SALObject.SFDPDummyCycle;
    if (status != EXTMEM_SFDP_ERROR_CONFIGDUMMY || read_status != HAL_OK ||
        (vcr != 0x1F && vcr != 0x00) ||
        Object->sfdp_private.ManuID != EXTMEM_MANUFACTURER_ISSI ||
        Object->sfdp_private.FlashSize != 25 ||
        Object->sfdp_private.DriverInfo.SpiPhyLink != PHY_LINK_8D8D8D ||
        Object->sfdp_public.MaxFreq == 0 || Object->sfdp_public.MaxFreq > 100000000)
        return status;
    uint32_t dummy = 16;
    return SAL_XSPI_MemoryConfig(&Object->sfdp_private.SALObject,
                                 PARAM_DUMMY_CYCLES, &dummy) == HAL_OK
           ? EXTMEM_SFDP_OK : EXTMEM_SFDP_ERROR_DRIVER;
}
