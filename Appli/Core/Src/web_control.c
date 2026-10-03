#include "web_control.h"
#include "cmsis_gcc.h"
#include <string.h>
static volatile WebTelemetry telemetry;
static volatile uint32_t requested_v,requested_i;
static volatile uint8_t limits_pending,output_pending,requested_output;
static volatile WebSequenceSnapshot sequence_snapshot;
static volatile WebSequenceCommand sequence_command;
static volatile uint8_t sequence_command_pending;
void WebControl_UpdateTelemetry(const WebTelemetry* v){uint32_t p=__get_PRIMASK();__disable_irq();memcpy((void*)&telemetry,v,sizeof(*v));if(!p)__enable_irq();}
void WebControl_GetTelemetry(WebTelemetry* v){uint32_t p=__get_PRIMASK();__disable_irq();memcpy(v,(const void*)&telemetry,sizeof(*v));if(!p)__enable_irq();}
void WebControl_RequestLimits(uint32_t v,uint32_t i){requested_v=v;requested_i=i;limits_pending=1;}
void WebControl_RequestOutput(uint8_t e){requested_output=e?1:0;output_pending=1;}
uint8_t WebControl_TakeLimits(uint32_t* v,uint32_t* i){if(!limits_pending)return 0;uint32_t p=__get_PRIMASK();__disable_irq();*v=requested_v;*i=requested_i;limits_pending=0;if(!p)__enable_irq();return 1;}
uint8_t WebControl_TakeOutput(uint8_t* e){if(!output_pending)return 0;uint32_t p=__get_PRIMASK();__disable_irq();*e=requested_output;output_pending=0;if(!p)__enable_irq();return 1;}
void WebControl_UpdateSequence(const WebSequenceSnapshot* v){uint32_t p=__get_PRIMASK();__disable_irq();memcpy((void*)&sequence_snapshot,v,sizeof(*v));if(!p)__enable_irq();}
void WebControl_GetSequence(WebSequenceSnapshot* v){uint32_t p=__get_PRIMASK();__disable_irq();memcpy(v,(const void*)&sequence_snapshot,sizeof(*v));if(!p)__enable_irq();}
static void request_sequence_command(const WebSequenceCommand* v){uint32_t p=__get_PRIMASK();__disable_irq();memcpy((void*)&sequence_command,v,sizeof(*v));sequence_command_pending=1;if(!p)__enable_irq();}
void WebControl_RequestSequenceStep(uint8_t index,const WebSequenceStep* step){WebSequenceCommand c={0};c.type=1;c.index=index;c.voltage_mv=step->voltage_mv;c.current_ma=step->current_ma;c.duration_ticks=step->duration_ticks;c.slew_mv_per_second=step->slew_mv_per_second;c.enabled=step->enabled;request_sequence_command(&c);}
void WebControl_RequestSequenceCount(uint8_t count){WebSequenceCommand c={0};c.type=2;c.value=count;request_sequence_command(&c);}
void WebControl_RequestSequenceLoop(uint8_t mode){WebSequenceCommand c={0};c.type=3;c.value=mode;request_sequence_command(&c);}
void WebControl_RequestSequenceRun(uint8_t running){WebSequenceCommand c={0};c.type=4;c.value=running?1:0;request_sequence_command(&c);}
uint8_t WebControl_TakeSequenceCommand(WebSequenceCommand* c){if(!sequence_command_pending)return 0;uint32_t p=__get_PRIMASK();__disable_irq();memcpy(c,(const void*)&sequence_command,sizeof(*c));sequence_command_pending=0;if(!p)__enable_irq();return 1;}
