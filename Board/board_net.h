#ifndef BOARD_NET_H
#define BOARD_NET_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void BoardNet_Init(void);
void BoardNet_Poll(uint32_t now);
/* Diagnostics: init, link, IPv4 network bytes, rx, tx, rx drops, tx errors,
   HAL error, DMA status, HTTP requests, max poll us, PHY special status, DHCP packets carrying the verified hostname. */
extern volatile uint32_t BoardNetDiagnostics[13];
#ifdef __cplusplus
}
#endif
#endif
