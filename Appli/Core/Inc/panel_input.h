#ifndef PANEL_INPUT_H
#define PANEL_INPUT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint16_t previous; int32_t edges; } PanelEncoder;
typedef struct {
    uint8_t candidate, stable, armed, held;
    uint32_t changed_ms, pressed_ms;
} PanelButton;
typedef struct { PanelEncoder encoder[2]; PanelButton button[3]; } PanelInput;
typedef struct {
    int8_t voltage, current;
    uint8_t voltage_step, current_step;
    uint8_t output; /* 1: short release; 2: one 2-second hold */
} PanelActions;
void panel_input_init(PanelInput *p, uint16_t voltage_count, uint16_t current_count,
                      uint8_t pressed_mask, uint32_t now);
PanelActions panel_input_update(PanelInput *p, uint16_t voltage_count, uint16_t current_count,
                               uint8_t pressed_mask, uint32_t now);
/* Active-high OUTPUT LED mask: bit0 red, bit1 green. Unknown feedback is dark. */
uint8_t panel_output_leds(int fresh, int output_on, int fault_latched, uint32_t now);
#ifdef __cplusplus
}
#endif
#endif
