#include "g4_uart.h"
#include <string.h>
#define RING_SIZE 2048U
static UART_HandleTypeDef *uart;
static G4Port *host;
static uint8_t chunk[128],ring[RING_SIZE];
static volatile uint16_t head,tail;
static volatile uint8_t reset_rx,tx_busy;
static char tx[G4_LINE_MAX+2];
static uint16_t tx_len;
static uint32_t retry_ms,tx_retry_ms;
static volatile G4UartDiagnostics diag;
static void arm_rx(void) {
  uint32_t irq;if(!uart)return;irq=__get_PRIMASK();__disable_irq();
  if(!diag.rx_active){if(HAL_UARTEx_ReceiveToIdle_IT(uart,chunk,sizeof(chunk))==HAL_OK)diag.rx_active=1;else ++diag.rx_start_failures;}
  if(!irq)__enable_irq();
}
void G4_UartInit(UART_HandleTypeDef *u,G4Port *p){uart=u;host=p;head=tail=0;reset_rx=tx_busy=0;tx_len=0;retry_ms=tx_retry_ms=0;memset((void*)&diag,0,sizeof(diag));if(host){host->need_session_init=1;host->init_phase=0;}arm_rx();}
void G4_UartProcess(uint32_t now) {
  unsigned i;if(!uart||!host)return;
  if(!diag.rx_active&&now-retry_ms>=10){retry_ms=now;arm_rx();}
  for(i=0;i<RING_SIZE;i++){
    uint8_t byte;uint32_t irq=__get_PRIMASK();__disable_irq();
    if(reset_rx){host->rx_len=0;host->rx_dropping=1;reset_rx=0;}
    if(tail==head){if(!irq)__enable_irq();break;}
    byte=ring[tail];tail=(tail+1)%RING_SIZE;if(!irq)__enable_irq();
    g4_rx_bytes(host,&byte,1,now);
  }
  if(!tx_busy){
    if(tx_len && (host->safety_pending || now-host->active_ms>G4_CMD_TIMEOUT_MS)) {
      host->response_id=host->active_id;host->response_state=host->safety_pending?3:4;
      tx_len=0;host->awaiting=0;
    }
    if(!tx_len&&g4_pop_tx(host,tx,sizeof(tx),now))tx_len=(uint16_t)strlen(tx);
    if(tx_len&&(now-tx_retry_ms>=10||!diag.tx_start_failures)){
      tx_retry_ms=now;tx_busy=1;
      if(HAL_UART_Transmit_IT(uart,(uint8_t*)tx,tx_len)==HAL_OK)tx_len=0;
      else{tx_busy=0;++diag.tx_start_failures;}
    }
  }
}
void G4_UartRxEvent(UART_HandleTypeDef *u,uint16_t size){
  uint16_t i;if(!uart||u!=uart)return;diag.rx_active=0;if(size>sizeof(chunk))size=sizeof(chunk);
  for(i=0;i<size;i++){uint16_t next=(head+1)%RING_SIZE;if(next==tail){++diag.rx_overflows;tail=head;reset_rx=1;}ring[head]=chunk[i];head=next;}arm_rx();
}
void G4_UartTxComplete(UART_HandleTypeDef *u){if(u==uart)tx_busy=0;}
void G4_UartError(UART_HandleTypeDef *u){if(!uart||u!=uart)return;++diag.uart_errors;diag.last_error=u->ErrorCode;diag.rx_active=0;tail=head;reset_rx=1;HAL_UART_AbortReceive(u);arm_rx();}
void G4_UartDiagnostics(G4UartDiagnostics *out){if(out)*out=diag;}
