/* LAN8742A/RMII + lwIP, polled in the UART/control task, never in the GUI ISR. */
#include "board_net.h"
#include "board_http.h"
#include "stm32h7rsxx_hal.h"
#include "psu_app.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include <string.h>

#define DMA_MEM __attribute__((section(".eth_dma"),aligned(32)))
#define NET_MEM __attribute__((section(".network_memory"),aligned(32)))
unsigned char BoardNetHeap[MEM_SIZE + 128] NET_MEM;
static ETH_DMADescTypeDef rxdesc[ETH_RX_DESC_CNT] DMA_MEM;
static ETH_DMADescTypeDef txdesc[ETH_TX_DESC_CNT] DMA_MEM;
static uint8_t rxbuf[ETH_RX_DESC_CNT][1536] DMA_MEM;
static uint8_t txbuffers[ETH_TX_DESC_CNT][1536] DMA_MEM;
static uint8_t txbusy[ETH_TX_DESC_CNT];
static uint8_t staging[1536];
static ETH_HandleTypeDef eth;
static struct netif interface;
static uint8_t mac[6], rxslot, ready, running, rebuild_dma;
static uint16_t rxlength;
static uint32_t last_link, dhcp_since;
volatile uint32_t BoardNetDiagnostics[13];

u32_t sys_now(void) { return HAL_GetTick(); }
static void pins(GPIO_TypeDef *port,uint32_t mask,unsigned alternate) {
    GPIO_InitTypeDef g={0};g.Pin=mask;g.Mode=GPIO_MODE_AF_PP;
    g.Pull=GPIO_NOPULL;g.Speed=GPIO_SPEED_FREQ_VERY_HIGH;g.Alternate=alternate;
    HAL_GPIO_Init(port,&g);
}
void HAL_ETH_MspInit(ETH_HandleTypeDef *h) {
    if(h->Instance!=ETH)return;
    __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();__HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();__HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_ETH1REF_CONFIG(RCC_ETH1REFCLKSOURCE_PHY);
    __HAL_RCC_ETH1MAC_CLK_ENABLE();__HAL_RCC_ETH1TX_CLK_ENABLE();__HAL_RCC_ETH1RX_CLK_ENABLE();
    pins(GPIOA,GPIO_PIN_2|GPIO_PIN_7,GPIO_AF11_ETH);  /* MDIO, CRS_DV */
    pins(GPIOB,GPIO_PIN_0|GPIO_PIN_1,GPIO_AF11_ETH);  /* TXD0, TXD1 */
    pins(GPIOC,GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5,GPIO_AF11_ETH); /* MDC, RXD0, RXD1 */
    pins(GPIOD,GPIO_PIN_7,GPIO_AF4_ETH); pins(GPIOG,GPIO_PIN_11,GPIO_AF11_ETH); /* REF_CLK, TX_EN */
    GPIO_InitTypeDef g={0};g.Pin=GPIO_PIN_4;g.Mode=GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_WritePin(GPIOE,GPIO_PIN_4,GPIO_PIN_SET);HAL_GPIO_Init(GPIOE,&g);
    /* Do not reset a PHY which has already negotiated a working link. */
}
void HAL_ETH_RxAllocateCallback(uint8_t **buffer) {
    *buffer=rxbuf[rxslot];rxslot=(rxslot+1)%ETH_RX_DESC_CNT;
}
void HAL_ETH_RxLinkCallback(void **start,void **end,uint8_t *buffer,uint16_t length) {
    /* HAL recycles DMA buffers before ReadData returns: copy before ownership
       is returned, not from the recycled buffers after ReadData. */
    if(!*start)rxlength=0;
    if((uint32_t)rxlength+length<=sizeof(staging)) {
        memcpy(staging+rxlength,buffer,length);rxlength+=length;
    } else rxlength=0xFFFF;
    *start=staging;*end=staging;
}
void HAL_ETH_TxFreeCallback(uint32_t *buffer) {
    for(unsigned i=0;i<ETH_TX_DESC_CNT;i++)
        if((void*)buffer==(void*)txbuffers[i]){txbusy[i]=0;break;}
}
static err_t send_packet(struct netif *n,struct pbuf *p) {
    (void)n;
    if(!running||p->tot_len>1536)return ERR_IF;
    HAL_ETH_ReleaseTxPacket(&eth);
    unsigned slot;
    for(slot=0;slot<ETH_TX_DESC_CNT;slot++)if(!txbusy[slot])break;
    if(slot==ETH_TX_DESC_CNT)return ERR_MEM;
    uint8_t *txbuf=txbuffers[slot];
    pbuf_copy_partial(p,txbuf,p->tot_len,0);
    /* Verify the actual DHCP wire option, not just the configured string. */
    if(p->tot_len>282 && txbuf[12]==8 && txbuf[13]==0 && txbuf[23]==17) {
        unsigned udp=14+(txbuf[14]&15)*4;
        if(udp+248U<p->tot_len && txbuf[udp]==0 && txbuf[udp+1]==68 &&
           txbuf[udp+2]==0 && txbuf[udp+3]==67) {
            for(unsigned pos=udp+248;pos<p->tot_len;) {
                unsigned option=txbuf[pos++];if(option==255)break;if(!option)continue;
                if(pos>=p->tot_len)break;
                unsigned length=txbuf[pos++];
                if(pos+length>p->tot_len)break;
                if(option==12 && length==10 && !memcmp(txbuf+pos,"LAB-PD-PSU",10))
                    ++BoardNetDiagnostics[12];
                pos+=length;
            }
        }
    }
    ETH_BufferTypeDef buffer={0};buffer.buffer=txbuf;buffer.len=p->tot_len;
    ETH_TxPacketConfigTypeDef config={0};
    config.Attributes=ETH_TX_PACKETS_FEATURES_CRCPAD;
    config.CRCPadCtrl=ETH_CRC_PAD_INSERT;config.Length=p->tot_len;config.TxBuffer=&buffer;
    config.pData=txbuf;txbusy[slot]=1;
    /* Queue DMA and return immediately. Reclaim by ownership polling; no ETH
       IRQ or busy wait can stall rendering/UART while a packet is on the wire. */
    eth.ErrorCode &= ~HAL_ETH_ERROR_BUSY;
    if(HAL_ETH_Transmit_IT(&eth,&config)!=HAL_OK) {
        txbusy[slot]=0;
        if(eth.ErrorCode&HAL_ETH_ERROR_BUSY)return ERR_MEM;
        ++BoardNetDiagnostics[6];return ERR_IF;
    }
    ++BoardNetDiagnostics[4];return ERR_OK;
}
static err_t net_init(struct netif *n) {
    n->name[0]='e';n->name[1]='n';n->hostname="LAB-PD-PSU";
    n->hwaddr_len=6;memcpy(n->hwaddr,mac,6);n->mtu=1500;
    n->flags=NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP;
    n->output=etharp_output;n->linkoutput=send_packet;return ERR_OK;
}
static void address_changed(struct netif *n) {
    const ip4_addr_t *ip=netif_ip4_addr(n),*mask=netif_ip4_netmask(n),*gw=netif_ip4_gw(n);
    BoardNetDiagnostics[2]=ip4_addr_get_u32(ip);
    if(!ip4_addr_isany_val(*ip)) {
        uint8_t a[4]={ip4_addr1(ip),ip4_addr2(ip),ip4_addr3(ip),ip4_addr4(ip)};
        uint8_t m[4]={ip4_addr1(mask),ip4_addr2(mask),ip4_addr3(mask),ip4_addr4(mask)};
        uint8_t g[4]={ip4_addr1(gw),ip4_addr2(gw),ip4_addr3(gw),ip4_addr4(gw)};
        psu_net_up();psu_net_bound(a,m,g);
    }
}
static void check_link(uint32_t now) {
    uint32_t status=0,special=0;
    if(HAL_ETH_ReadPHYRegister(&eth,0,1,&status)!=HAL_OK)return;
    if(HAL_ETH_ReadPHYRegister(&eth,0,1,&status)!=HAL_OK)return;
    HAL_ETH_ReadPHYRegister(&eth,0,31,&special);BoardNetDiagnostics[11]=special;
    uint8_t up=(status&4)!=0;
    if(up==running)return;
    if(up) {
        if(rebuild_dma) {
            /* A cable loss may leave owned TX descriptors. Reset only the MAC
               before reusing buffers; the PHY remains negotiated and running. */
            memset(&eth.TxDescList,0,sizeof(eth.TxDescList));
            memset(&eth.RxDescList,0,sizeof(eth.RxDescList));
            memset(txbusy,0,sizeof(txbusy));rxslot=0;
            if(HAL_ETH_Init(&eth)!=HAL_OK)return;
            rebuild_dma=0;
        }
        ETH_MACConfigTypeDef config={0};HAL_ETH_GetMACConfig(&eth,&config);
        config.Speed=(special&8)?ETH_SPEED_100M:ETH_SPEED_10M;
        config.DuplexMode=(special&16)?ETH_FULLDUPLEX_MODE:ETH_HALFDUPLEX_MODE;
        config.ChecksumOffload=DISABLE;
        HAL_ETH_SetMACConfig(&eth,&config);
        if(HAL_ETH_Start(&eth)!=HAL_OK)return;
        running=1;netif_set_link_up(&interface);netif_set_up(&interface);
        psu_net_up();dhcp_start(&interface);dhcp_since=now;
    } else {
        dhcp_stop(&interface);netif_set_link_down(&interface);netif_set_down(&interface);
        netif_set_addr(&interface,IP4_ADDR_ANY4,IP4_ADDR_ANY4,IP4_ADDR_ANY4);
        HAL_ETH_Stop(&eth);running=0;rebuild_dma=1;psu_net_down();
    }
    BoardNetDiagnostics[1]=running;
}
void BoardNet_Init(void) {
    memset((void*)BoardNetDiagnostics,0,sizeof(BoardNetDiagnostics));
    __HAL_RCC_SRAM1_CLK_ENABLE();__HAL_RCC_SRAM2_CLK_ENABLE();
    memset(rxdesc,0,sizeof(rxdesc));memset(txdesc,0,sizeof(txdesc));
    uint32_t a=HAL_GetUIDw0(),b=HAL_GetUIDw1(),c=HAL_GetUIDw2();
    /* Stable locally administered unicast address, derived from MCU UID. */
    mac[0]=0x02;mac[1]=0x4C;mac[2]=(a^b)>>8;mac[3]=a^c;mac[4]=b>>16;mac[5]=c>>24;
    eth.Instance=ETH;eth.Init.MACAddr=mac;eth.Init.MediaInterface=HAL_ETH_RMII_MODE;
    eth.Init.RxDesc=rxdesc;eth.Init.TxDesc=txdesc;eth.Init.RxBuffLen=1536;
    if(HAL_ETH_Init(&eth)!=HAL_OK){BoardNetDiagnostics[0]=0xE001;BoardNetDiagnostics[7]=eth.ErrorCode;BoardNetDiagnostics[8]=ETH->DMAMR;return;}
    uint32_t id1=0,id2=0;HAL_ETH_ReadPHYRegister(&eth,0,2,&id1);HAL_ETH_ReadPHYRegister(&eth,0,3,&id2);
    if(id1!=7||(id2&0xFFF0)!=0xC130){BoardNetDiagnostics[0]=0xE002;return;}
    lwip_init();
    if(!netif_add(&interface,IP4_ADDR_ANY4,IP4_ADDR_ANY4,IP4_ADDR_ANY4,0,net_init,ethernet_input)) {
        BoardNetDiagnostics[0]=0xE003;return;
    }
    netif_set_default(&interface);netif_set_status_callback(&interface,address_changed);
    psu_net_set_mac(mac);psu_net_configured(1);BoardHttp_Init();
    ready=1;BoardNetDiagnostics[0]=1;check_link(HAL_GetTick());
}
void BoardNet_Poll(uint32_t now) {
    if(!ready)return;
    uint32_t start=DWT->CYCCNT;
    HAL_ETH_ReleaseTxPacket(&eth);
    /* Bounded work prevents broadcast traffic starving UART or display. */
    for(unsigned i=0;running&&i<8;i++) {
        void *packet=0;
        if(HAL_ETH_ReadData(&eth,&packet)!=HAL_OK)break;
        if(rxlength>=14&&rxlength<=sizeof(staging)) {
            struct pbuf *p=pbuf_alloc(PBUF_RAW,rxlength,PBUF_POOL);
            if(p) {
                pbuf_take(p,staging,rxlength);++BoardNetDiagnostics[3];
                if(interface.input(p,&interface)!=ERR_OK)pbuf_free(p);
            } else ++BoardNetDiagnostics[5];
        } else ++BoardNetDiagnostics[5];
        if((DWT->CYCCNT-start)>300000)break;
    }
    sys_check_timeouts();BoardHttp_Poll(now);
    if(now-last_link>=250){last_link=now;check_link(now);}
    if(running&&ip4_addr_isany_val(*netif_ip4_addr(&interface))&&now-dhcp_since>30000) {
        dhcp_stop(&interface);dhcp_start(&interface);dhcp_since=now;
    }
    BoardNetDiagnostics[7]=eth.ErrorCode;BoardNetDiagnostics[8]=ETH->DMACSR;
    uint32_t us=(DWT->CYCCNT-start)/(SystemCoreClock/1000000);
    if(us>BoardNetDiagnostics[10])BoardNetDiagnostics[10]=us;
}
