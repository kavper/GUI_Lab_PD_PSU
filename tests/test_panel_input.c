#include "panel_input.h"
#include <assert.h>
#include <stdio.h>
static PanelInput input;
static PanelActions tick(uint16_t v, uint16_t i, uint8_t keys, uint32_t ms)
{ return panel_input_update(&input, v, i, keys, ms); }
int main(void)
{
    panel_input_init(&input,0,0,0,0);
    assert(!tick(1,65535,0,1).voltage);
    assert(!tick(2,65534,0,2).voltage);
    assert(!tick(3,65533,0,3).voltage);
    PanelActions a=tick(4,65532,0,4);
    assert(a.voltage==1 && a.current==-1);
    /* Hardware counter bounce reverses partial travel, not full detents. */
    assert(!tick(5,65532,0,5).voltage); assert(!tick(4,65532,0,6).voltage);
    assert(!tick(5,65532,0,7).voltage); assert(!tick(6,65532,0,8).voltage);
    assert(!tick(7,65532,0,9).voltage); assert(tick(8,65532,0,10).voltage==1);
    /* Both signed directions continue correctly through 16-bit wrap. */
    panel_input_init(&input,65534,1,0,0);
    tick(65535,0,0,1); tick(0,65535,0,2); tick(1,65534,0,3);
    a=tick(2,65533,0,4); assert(a.voltage==1 && a.current==-1);
    /* A GUI delay does not lose timer counts; large bursts drain in bounded batches. */
    panel_input_init(&input,0,0,0,0);
    assert(tick(160,0,0,1).voltage==16);
    assert(tick(160,0,0,2).voltage==16);
    assert(tick(160,0,0,3).voltage==8);
    assert(!tick(160,0,0,4).voltage);
    /* Debounced short OUTPUT fires on release, never when starting a hold. */
    panel_input_init(&input,3,3,0,0);
    assert(!tick(3,3,4,10).output); assert(!tick(3,3,0,15).output);
    assert(!tick(3,3,4,18).output); assert(!tick(3,3,4,48).output);
    assert(!tick(3,3,0,100).output); assert(tick(3,3,0,130).output==1);
    assert(!tick(3,3,0,160).output);
    /* Long hold emits one shutdown and consumes the release. */
    panel_input_init(&input,3,3,0,0);
    tick(3,3,4,10); tick(3,3,4,40);
    assert(!tick(3,3,4,2009).output); assert(tick(3,3,4,2010).output==2);
    assert(!tick(3,3,4,2200).output); tick(3,3,0,2210);
    assert(!tick(3,3,0,2240).output);
    /* Releasing just before 2 seconds stays a short click during debounce. */
    panel_input_init(&input,3,3,0,0); tick(3,3,4,10); tick(3,3,4,40);
    tick(3,3,0,2009); assert(!tick(3,3,0,2010).output);
    assert(tick(3,3,0,2039).output==1);
    /* A held boot key cannot produce an accidental ON; unsigned wrap is safe. */
    panel_input_init(&input,3,3,4,0);
    assert(!tick(3,3,4,3000).output); tick(3,3,0,3010);
    assert(!tick(3,3,0,3040).output);
    uint32_t start=0xffffff00U;
    panel_input_init(&input,3,3,0,start); tick(3,3,4,start+10); tick(3,3,4,start+40);
    assert(tick(3,3,4,start+2010).output==2);
    panel_input_init(&input,3,3,0,0); tick(3,3,3,10); tick(3,3,3,40);
    tick(3,3,0,60); a=tick(3,3,0,90);
    assert(a.voltage_step==1 && a.current_step==1 && !a.output);
    assert(panel_output_leds(0,0,0,0)==0);
    assert(panel_output_leds(0,1,0,0)==0);
    assert(panel_output_leds(1,0,0,0)==1);
    assert(panel_output_leds(1,1,0,0)==2);
    assert(panel_output_leds(1,1,1,250)==1);
    assert(panel_output_leds(1,1,1,500)==0);
    assert(panel_output_leds(0,1,1,750)==1);
    puts("PASS: encoders, bounce, counter wrap, short/2s hold, boot-held key, confirmed output LEDs and latched fault");
}
