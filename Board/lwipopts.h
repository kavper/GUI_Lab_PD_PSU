/* Board-owned lwIP configuration. All raw API calls run in defaultTask. */
#ifndef BOARD_LWIPOPTS_H
#define BOARD_LWIPOPTS_H
#include <stdint.h>
#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define MEM_ALIGNMENT 4
#define MEM_SIZE (96 * 1024)
extern unsigned char BoardNetHeap[];
#define LWIP_RAM_HEAP_POINTER BoardNetHeap
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_ETHERNET 1
#define LWIP_ARP 1
#define LWIP_ICMP 1
#define LWIP_DHCP 1
#define LWIP_DHCP_DISCOVER_ADD_HOSTNAME 1
#define LWIP_DHCP_CHECK_LINK_UP 1
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_NETCONN 0
#define LWIP_SOCKET 0
#define LWIP_TCP 1
#define LWIP_UDP 1
#define LWIP_DNS 0
#define LWIP_STATS 1
#define LWIP_STATS_DISPLAY 0
#define TCP_MSS 1460
#define TCP_WND (8 * TCP_MSS)
#define TCP_SND_BUF (4 * TCP_MSS)
#define TCP_SND_QUEUELEN 32
#define MEMP_NUM_TCP_SEG 96
#define MEMP_NUM_TCP_PCB 8
#define MEMP_NUM_TCP_PCB_LISTEN 2
#define PBUF_POOL_SIZE 24
#define PBUF_POOL_BUFSIZE 1536
#define MEMP_NUM_SYS_TIMEOUT 12
#define LWIP_TCP_KEEPALIVE 1
#define IP_REASSEMBLY 0
#define IP_FRAG 0
/* Software checksums: safe before validating the MAC checksum offload path. */
#define CHECKSUM_GEN_IP 1
#define CHECKSUM_GEN_UDP 1
#define CHECKSUM_GEN_TCP 1
#define CHECKSUM_CHECK_IP 1
#define CHECKSUM_CHECK_UDP 1
#define CHECKSUM_CHECK_TCP 1
#endif
