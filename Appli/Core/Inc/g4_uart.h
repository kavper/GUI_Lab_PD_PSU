#ifndef G4_UART_H
#define G4_UART_H
#include "stm32h7rsxx_hal.h"
#include "g4_ascii.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint32_t uart_errors,last_error,rx_start_failures,rx_overflows,tx_start_failures;uint8_t rx_active;} G4UartDiagnostics;
void G4_UartInit(UART_HandleTypeDef *uart,G4Port *port);
void G4_UartProcess(uint32_t now);
void G4_UartRxEvent(UART_HandleTypeDef *uart,uint16_t size);
void G4_UartTxComplete(UART_HandleTypeDef *uart);
void G4_UartError(UART_HandleTypeDef *uart);
void G4_UartDiagnostics(G4UartDiagnostics *out);
#ifdef __cplusplus
}
#endif
#endif
