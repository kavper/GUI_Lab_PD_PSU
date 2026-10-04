#include "g4_uart.h"
#include <string.h>
/* H7S7 HPDMA linked-list circular RX. Dedicated cache lines in AXI SRAM. */
#define RX_SIZE 512u
static uint8_t rx[RX_SIZE] __attribute__((aligned(32)));
static uint8_t tx[128] __attribute__((aligned(32)));
static DMA_NodeTypeDef rx_node __attribute__((aligned(32),section("Nemagfx_Memory_Pool_Buffer")));
static DMA_QListTypeDef rx_list;
static DMA_HandleTypeDef dma_rx,dma_tx;
static UART_HandleTypeDef *uart;
static G4Port *host;
static uint16_t tail,tx_len;
static volatile uint32_t laps;
static uint32_t consumed_laps,last_poll;
static volatile uint8_t restart,tx_busy;
static volatile G4UartDiagnostics diag;
static void invalidate(void *p,unsigned n){uintptr_t a=(uintptr_t)p&~(uintptr_t)31;SCB_InvalidateDCache_by_Addr((uint32_t*)a,(int32_t)(((uintptr_t)p+n+31-a)&~(uintptr_t)31));}
static void clean(void *p,unsigned n){uintptr_t a=(uintptr_t)p&~(uintptr_t)31;SCB_CleanDCache_by_Addr((uint32_t*)a,(int32_t)(((uintptr_t)p+n+31-a)&~(uintptr_t)31));}
static int setup_dma(void){DMA_NodeConfTypeDef c={0};__HAL_RCC_HPDMA1_CLK_ENABLE();
 c.NodeType=DMA_HPDMA_LINEAR_NODE;c.Init.Request=HPDMA1_REQUEST_UART7_RX;c.Init.BlkHWRequest=DMA_BREQ_SINGLE_BURST;c.Init.Direction=DMA_PERIPH_TO_MEMORY;c.Init.SrcInc=DMA_SINC_FIXED;c.Init.DestInc=DMA_DINC_INCREMENTED;c.Init.SrcDataWidth=DMA_SRC_DATAWIDTH_BYTE;c.Init.DestDataWidth=DMA_DEST_DATAWIDTH_BYTE;c.Init.SrcBurstLength=1;c.Init.DestBurstLength=1;c.Init.TransferAllocatedPort=DMA_SRC_ALLOCATED_PORT1|DMA_DEST_ALLOCATED_PORT0;c.Init.TransferEventMode=DMA_TCEM_BLOCK_TRANSFER;c.Init.Mode=DMA_NORMAL;c.SrcAddress=(uint32_t)&uart->Instance->RDR;c.DstAddress=(uint32_t)rx;c.DataSize=RX_SIZE;
 if(HAL_DMAEx_List_BuildNode(&c,&rx_node)!=HAL_OK||HAL_DMAEx_List_InsertNode_Tail(&rx_list,&rx_node)!=HAL_OK||HAL_DMAEx_List_SetCircularMode(&rx_list)!=HAL_OK)return 0;
 dma_rx.Instance=HPDMA1_Channel2;dma_rx.InitLinkedList.Priority=DMA_HIGH_PRIORITY;dma_rx.InitLinkedList.LinkStepMode=DMA_LSM_FULL_EXECUTION;dma_rx.InitLinkedList.LinkAllocatedPort=DMA_LINK_ALLOCATED_PORT0;dma_rx.InitLinkedList.TransferEventMode=DMA_TCEM_BLOCK_TRANSFER;dma_rx.InitLinkedList.LinkedListMode=DMA_LINKEDLIST_CIRCULAR;
 if(HAL_DMAEx_List_Init(&dma_rx)!=HAL_OK||HAL_DMAEx_List_LinkQ(&dma_rx,&rx_list)!=HAL_OK)return 0;
 dma_tx.Instance=HPDMA1_Channel3;dma_tx.Init=c.Init;dma_tx.Init.Request=HPDMA1_REQUEST_UART7_TX;dma_tx.Init.Direction=DMA_MEMORY_TO_PERIPH;dma_tx.Init.TransferAllocatedPort=DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT1;dma_tx.Init.SrcInc=DMA_SINC_INCREMENTED;dma_tx.Init.DestInc=DMA_DINC_FIXED;if(HAL_DMA_Init(&dma_tx)!=HAL_OK)return 0;
 __HAL_LINKDMA(uart,hdmarx,dma_rx);__HAL_LINKDMA(uart,hdmatx,dma_tx);
 HAL_NVIC_SetPriority(HPDMA1_Channel2_IRQn,6,0);HAL_NVIC_EnableIRQ(HPDMA1_Channel2_IRQn);HAL_NVIC_SetPriority(HPDMA1_Channel3_IRQn,6,0);HAL_NVIC_EnableIRQ(HPDMA1_Channel3_IRQn);return 1;
}
void HPDMA1_Channel2_IRQHandler(void){HAL_DMA_IRQHandler(&dma_rx);}
void HPDMA1_Channel3_IRQHandler(void){HAL_DMA_IRQHandler(&dma_tx);}
static void arm(void){__HAL_UART_DISABLE_IT(uart,UART_IT_IDLE);HAL_UART_AbortReceive(uart);__HAL_UART_CLEAR_IDLEFLAG(uart);__HAL_UART_SEND_REQ(uart,UART_RXDATA_FLUSH_REQUEST);invalidate(rx,sizeof(rx));tail=0;laps=consumed_laps=0;g4_reset_parser(host);__DSB();if(HAL_UARTEx_ReceiveToIdle_DMA(uart,rx,RX_SIZE)==HAL_OK){diag.rx_active=1;restart=0;}else{diag.rx_active=0;diag.rx_start_failures++;}}
void G4_UartInit(UART_HandleTypeDef *u,G4Port *p){uart=u;host=p;memset((void*)&diag,0,sizeof(diag));tx_busy=0;tx_len=0;restart=1;if(setup_dma())arm();}
void G4_UartProcess(uint32_t now){uint32_t epoch,w,absolute,read;uint8_t bytes[RX_SIZE];unsigned n=0;if(!uart||!host||!uart->hdmarx)return;if(restart||!diag.rx_active){arm();last_poll=now;goto transmit;}
 /* Epoch and CBR1 must describe the same lap. TC updates only the epoch. */
 {uint32_t irq=__get_PRIMASK();__disable_irq();epoch=laps;w=RX_SIZE-__HAL_DMA_GET_COUNTER(uart->hdmarx);if(!irq)__enable_irq();}
 if(w>=RX_SIZE){w=0;}
 absolute=epoch*RX_SIZE+w;read=consumed_laps*RX_SIZE+tail;
 if(absolute-read>=RX_SIZE||now-last_poll>25){diag.rx_overflows++;host->overflow_count++;g4_reset_parser(host);tail=(uint16_t)w;consumed_laps=epoch;}else{invalidate(rx,sizeof(rx));while(read<absolute&&n<RX_SIZE){bytes[n++]=rx[tail];tail=(tail+1)%RX_SIZE;read++;if(!tail)consumed_laps++;}if(n)g4_rx_bytes(host,bytes,n,now);}last_poll=now;
 transmit:
 if(!tx_busy){if(!tx_len)tx_len=(uint16_t)g4_pop_frame(host,tx,sizeof(tx),now);if(tx_len){clean(tx,sizeof(tx));tx_busy=1;if(HAL_UART_Transmit_DMA(uart,tx,tx_len)==HAL_OK)tx_len=0;else{tx_busy=0;diag.tx_start_failures++;}}}}
void G4_UartRxEvent(UART_HandleTypeDef *u,uint16_t size){(void)size;if(u==uart&&HAL_UARTEx_GetRxEventType(u)==HAL_UART_RXEVENT_TC)laps++;}
void G4_UartTxComplete(UART_HandleTypeDef *u){if(u==uart)tx_busy=0;}
void G4_UartError(UART_HandleTypeDef *u){if(u!=uart)return;diag.uart_errors++;diag.last_error=u->ErrorCode;restart=1;diag.rx_active=0;__HAL_UART_DISABLE_IT(u,UART_IT_IDLE);__HAL_UART_DISABLE_IT(u,UART_IT_ERR);__HAL_UART_DISABLE_IT(u,UART_IT_RXNE);__HAL_UART_DISABLE_IT(u,UART_IT_RXFT);__HAL_UART_DISABLE_IT(u,UART_IT_PE);__HAL_UART_CLEAR_IDLEFLAG(u);__HAL_UART_CLEAR_OREFLAG(u);__HAL_UART_CLEAR_FEFLAG(u);__HAL_UART_CLEAR_NEFLAG(u);if(u->gState!=HAL_UART_STATE_BUSY_TX)tx_busy=0;}
void G4_UartDiagnostics(G4UartDiagnostics *out){if(out)*out=diag;}
