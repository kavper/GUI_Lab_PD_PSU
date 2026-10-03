#include "stm32h7rsxx_hal.h"
#include <string.h>

/*
 * Low-level Ethernet glue for the H7RS HAL in
 * Drivers/STM32H7RSxx_HAL_Driver/Inc/stm32h7rsxx_hal_eth.h.
 *
 * The header declares ETH_DMADescTypeDef, ETH_HandleTypeDef,
 * ETH_BufferTypeDef, ETH_MACConfigTypeDef, ETH_TxPacketConfigTypeDef,
 * HAL_ETH_Init and HAL_ETH_Transmit. ETH_TxPacketConfig is only the
 * legacy alias of ETH_TxPacketConfigTypeDef in stm32_hal_legacy.h.
 * Descriptor counts are ETH_RX_DESC_CNT and ETH_TX_DESC_CNT.
 */

ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((aligned(32)));
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((aligned(32)));
ETH_HandleTypeDef EthHandle;

#define ETH_RX_BUFFER_SIZE 1536U
static uint8_t Rx_Buff[ETH_RX_DESC_CNT][ETH_RX_BUFFER_SIZE] __attribute__((aligned(32)));

static void ethernetif_rx_allocate(uint8_t **buffer)
{
  static uint32_t slot;
  *buffer = Rx_Buff[slot];
  slot++;
  if (slot >= ETH_RX_DESC_CNT)
    slot = 0U;
}

int8_t ethernetif_init(void)
{
  ETH_MACConfigTypeDef mac;
  uint8_t macaddress[6] = {0x00, 0x80, 0xE1, 0x00, 0x00, 0x01};

  EthHandle.Instance = ETH;
  EthHandle.Init.MACAddr = macaddress;
  EthHandle.Init.MediaInterface = HAL_ETH_RMII_MODE;
  EthHandle.Init.RxDesc = DMARxDscrTab;
  EthHandle.Init.TxDesc = DMATxDscrTab;
  EthHandle.Init.RxBuffLen = ETH_RX_BUFFER_SIZE;

  if (HAL_ETH_Init(&EthHandle) != HAL_OK)
    return -1;
  if (HAL_ETH_RegisterRxAllocateCallback(&EthHandle, ethernetif_rx_allocate) != HAL_OK)
    return -1;

  memset(&mac, 0, sizeof(mac));
  if (HAL_ETH_GetMACConfig(&EthHandle, &mac) != HAL_OK)
    return -1;
  mac.Speed = ETH_SPEED_100M;
  mac.DuplexMode = ETH_FULLDUPLEX_MODE;
  if (HAL_ETH_SetMACConfig(&EthHandle, &mac) != HAL_OK)
    return -1;
  if (HAL_ETH_Start(&EthHandle) != HAL_OK)
    return -1;
  return 0;
}

int8_t ethernetif_output(const uint8_t *frame, uint32_t length)
{
  ETH_BufferTypeDef txbuf;
  ETH_TxPacketConfigTypeDef txcfg;

  if (frame == 0 || length == 0U || length > ETH_MAX_PAYLOAD)
    return -1;

  memset(&txbuf, 0, sizeof(txbuf));
  memset(&txcfg, 0, sizeof(txcfg));
  txbuf.buffer = (uint8_t *)frame;
  txbuf.len = length;
  txbuf.next = 0;
  txcfg.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  txcfg.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  txcfg.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  txcfg.Length = length;
  txcfg.TxBuffer = &txbuf;
  txcfg.pData = 0;

  if (HAL_ETH_Transmit(&EthHandle, &txcfg, 100U) != HAL_OK)
    return -1;
  return 0;
}

void ethernetif_input_poll(void)
{
  void *buff = 0;
  (void)HAL_ETH_ReadData(&EthHandle, &buff);
  (void)buff;
}
