#include "panel_input.h"
#include <string.h>
static int8_t encoder_update(PanelEncoder *e, uint16_t count)
{
    /* TIM3/TIM4 count quadrature edges in hardware, including during GUI drawing.
     * Signed 16-bit subtraction handles either direction across CNT rollover. */
    e->edges += (int16_t)(uint16_t)(count - e->previous);
    e->previous = count;
    int32_t detents = e->edges / 4;
    if (detents > 16) detents = 16;
    if (detents < -16) detents = -16;
    e->edges -= detents * 4; /* Keep partial travel/bounce and any burst backlog. */
    return (int8_t)detents;
}
static uint8_t button_update(PanelButton *b, uint8_t pressed, uint8_t long_press, uint32_t now)
{
    uint8_t event = 0;
    if (pressed != b->candidate) { b->candidate = pressed; b->changed_ms = now; }
    if (b->candidate != b->stable && (uint32_t)(now - b->changed_ms) >= 30) {
        b->stable = b->candidate;
        if (b->stable) { b->pressed_ms = b->changed_ms; b->held = 0; }
        else {
            if (b->armed && !b->held)
                event = long_press && (uint32_t)(b->changed_ms-b->pressed_ms) >= 2000 ? 2 : 1;
            b->armed = 1;
        }
    }
    if (long_press && pressed && b->stable && b->armed && !b->held &&
        (uint32_t)(now - b->pressed_ms) >= 2000) {
        b->held = 1; event = 2;
    }
    return event;
}
void panel_input_init(PanelInput *p, uint16_t voltage_count, uint16_t current_count,
                      uint8_t pressed_mask, uint32_t now)
{
    memset(p, 0, sizeof(*p));
    p->encoder[0].previous = voltage_count;
    p->encoder[1].previous = current_count;
    for (unsigned i=0; i<3; ++i) {
        PanelButton *b = &p->button[i];
        b->candidate = b->stable = (pressed_mask >> i) & 1;
        b->armed = !b->stable; /* A key held at boot cannot accidentally turn output on. */
        b->changed_ms = b->pressed_ms = now;
    }
}
PanelActions panel_input_update(PanelInput *p, uint16_t voltage_count, uint16_t current_count,
                               uint8_t pressed_mask, uint32_t now)
{
    PanelActions a;
    a.voltage = encoder_update(&p->encoder[0], voltage_count);
    a.current = encoder_update(&p->encoder[1], current_count);
    a.voltage_step = button_update(&p->button[0], pressed_mask & 1, 0, now);
    a.current_step = button_update(&p->button[1], (pressed_mask >> 1) & 1, 0, now);
    a.output = button_update(&p->button[2], (pressed_mask >> 2) & 1, 1, now);
    return a;
}
