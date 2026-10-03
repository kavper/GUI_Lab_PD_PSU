#ifndef WEB_CONTROL_H
#define WEB_CONTROL_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t vin_mv,vout_mv,set_voltage_mv,set_current_ma; int32_t iout_ua; int16_t mos_deci_c,pcb_deci_c; uint8_t mode,connected,output_on; } WebTelemetry;
typedef struct { uint32_t voltage_mv,current_ma,duration_ticks,slew_mv_per_second; uint8_t enabled; } WebSequenceStep;
typedef struct { WebSequenceStep steps[12]; uint32_t revision; uint8_t count,loop_mode,running,running_step,completed_loops,state; } WebSequenceSnapshot;
typedef struct { uint32_t voltage_mv,current_ma,duration_ticks,slew_mv_per_second; uint8_t type,index,value,enabled; } WebSequenceCommand;
void WebControl_UpdateTelemetry(const WebTelemetry* value);
void WebControl_GetTelemetry(WebTelemetry* value);
void WebControl_RequestLimits(uint32_t voltage_mv,uint32_t current_ma);
void WebControl_RequestOutput(uint8_t enabled);
uint8_t WebControl_TakeLimits(uint32_t* voltage_mv,uint32_t* current_ma);
uint8_t WebControl_TakeOutput(uint8_t* enabled);
void WebControl_UpdateSequence(const WebSequenceSnapshot* value);
void WebControl_GetSequence(WebSequenceSnapshot* value);
void WebControl_RequestSequenceStep(uint8_t index,const WebSequenceStep* step);
void WebControl_RequestSequenceCount(uint8_t count);
void WebControl_RequestSequenceLoop(uint8_t loop_mode);
void WebControl_RequestSequenceRun(uint8_t running);
uint8_t WebControl_TakeSequenceCommand(WebSequenceCommand* command);
#ifdef __cplusplus
}
#endif
#endif
