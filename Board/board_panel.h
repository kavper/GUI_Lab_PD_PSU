#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { BOARD_VOLTAGE = 1, BOARD_CURRENT, BOARD_VOLTAGE_STEP, BOARD_CURRENT_STEP };
typedef struct { uint8_t kind; int8_t delta; } BoardPanelEvent;
void BoardPanel_Init(void);
void BoardPanel_Poll(uint32_t now);
int BoardPanel_Take(BoardPanelEvent *event);
#ifdef __cplusplus
}
#endif
