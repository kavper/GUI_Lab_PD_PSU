#include "psu_charger.h"
#include "psu_seq.h"
#include <assert.h>
#include <stdio.h>
static unsigned mv,ma,on_calls,off_calls,permit_calls,ready,applied,limits_calls;
static void limits(uint32_t v,uint32_t i,void*u){(void)u;mv=v;ma=i;++limits_calls;}
static void output(int on,void*u){(void)u;if(on){assert(limits_calls>0);on_calls++;}else off_calls++;}
static void permit(int on,void*u){(void)on;(void)u;permit_calls++;}
static int readback(uint32_t*v,uint32_t*i,void*u){(void)u;*v=mv;*i=ma;return applied;}
static int allowed(void*u){(void)u;return 1;}
static int output_ready(int on,void*u){(void)u;return on?ready:1;}
int main(void){
 PsuCharger c;PsuChgSense s={0};PsuSequencer q;unsigned t;
 psu_chg_init(&c);c.io.limits=limits;c.io.output=output;c.io.permit=permit;
 c.profile.confirmed=c.profile.polarity_checked=1;
 s.pack_mv=3700;s.telemetry_ok=s.start_allowed=1;
 assert(psu_chg_start(&c,&c.profile,&s,1));
 psu_chg_tick(&c,&s,2);assert(c.state==CHG_STARTING&&mv==4200&&ma==500&&!on_calls&&!permit_calls);
 psu_chg_tick(&c,&s,50);assert(!on_calls);
 s.limits_applied=1;psu_chg_tick(&c,&s,100);assert(on_calls==1&&c.state==CHG_STARTING);
 psu_chg_tick(&c,&s,1000);assert(c.running&&c.state==CHG_STARTING&&!permit_calls);
 s.permit=s.output_ready=1;psu_chg_tick(&c,&s,1100);assert(c.state==CHG_CC);
 s.permit=0;psu_chg_tick(&c,&s,1200);assert(!c.running&&c.state==CHG_FAULT&&off_calls==1);
 s.permit=1;psu_chg_tick(&c,&s,1300);assert(!c.running&&on_calls==1);
 s.start_allowed=s.permit=0;assert(!psu_chg_start(&c,&c.profile,&s,1400));
 s.start_allowed=1;s.output_ready=s.limits_applied=0;assert(psu_chg_start(&c,&c.profile,&s,1500));
 psu_chg_tick(&c,&s,1510);psu_chg_tick(&c,&s,6520);assert(!c.running&&c.state==CHG_FAULT);
 puts("PASS charger: no initial PERMIT required; SET before ON; delayed G4/G0 readiness; no forced PERMIT; running loss trips; timeout and no automatic restart");
 psu_chg_init(&c);c.io.limits=limits;c.io.output=output;
 c.profile.confirmed=c.profile.polarity_checked=1;c.profile.cc_ma=1000;c.profile.capacity_mah=1;
 s.pack_mv=2500;s.telemetry_ok=s.start_allowed=1;s.permit=s.output_ready=0;
 assert(psu_chg_start(&c,&c.profile,&s,7000));psu_chg_tick(&c,&s,7010);assert(ma==100);
 s.limits_applied=1;psu_chg_tick(&c,&s,7020);s.permit=s.output_ready=1;
 psu_chg_tick(&c,&s,7030);assert(c.state==CHG_PRECHARGE);
 s.pack_mv=3700;psu_chg_tick(&c,&s,7600);psu_chg_tick(&c,&s,7610);
 assert(c.state==CHG_CC&&ma==1000); /* End threshold 100 mA never sets CC. */
 s.pack_mv=4200;s.current_ma=500;psu_chg_tick(&c,&s,8200);psu_chg_tick(&c,&s,8210);
 assert(c.state==CHG_CV&&ma==1000);
 s.current_ma=50;s.limits_applied=0;psu_chg_tick(&c,&s,8215);assert(c.state==CHG_CV);s.limits_applied=1;psu_chg_tick(&c,&s,8220);psu_chg_tick(&c,&s,8800);psu_chg_tick(&c,&s,8810);
 assert(c.state==CHG_COMPLETE&&!c.running);
 puts("PASS explicit 1 A independent of capacity: low battery 0.1 A -> healthy CC 1 A -> CV -> end threshold stops");
 on_calls=off_calls=0;psu_seq_init(&q);q.io.limits=limits;q.io.output=output;q.io.applied=readback;q.io.permit_ok=allowed;q.io.output_ready=output_ready;
 q.steps[0].output_action=PSU_STEP_ON;q.steps[0].time_ms=100;q.mode=PSU_SEQ_N;q.loops_requested=2;
 assert(psu_seq_start(&q,0));psu_seq_tick(&q,0);assert(!on_calls);
 applied=1;psu_seq_tick(&q,10);assert(on_calls==1&&q.waiting_output);
 psu_seq_tick(&q,500);assert(q.run==PSU_SEQ_RUN&&q.step_index==0);
 ready=1;psu_seq_tick(&q,600);psu_seq_tick(&q,699);assert(q.step_index==0);
 for(t=700;t<1500;t+=10)psu_seq_tick(&q,t);
 assert(q.run==PSU_SEQ_DONE&&q.loop_index==2&&off_calls==1&&on_calls==2);
 q.mode=PSU_SEQ_INFINITE;assert(psu_seq_start(&q,2000));for(t=2000;t<5000;t+=10)psu_seq_tick(&q,t);
 assert(q.run==PSU_SEQ_RUN&&q.loop_index>10);psu_seq_stop(&q,5000,0);assert(q.run==PSU_SEQ_IDLE);
 q.steps[0].enabled=0;assert(!psu_seq_start(&q,5010));
 puts("PASS sequencer: SET/readback before ON; duration begins at confirmed output; two cycles; continuous; explicit stop; reject empty sequence");
 /* Both slopes use the preceding voltage, including a zero target. */
 psu_seq_init(&q);q.io.limits=limits;q.io.output=output;q.io.applied=readback;q.io.permit_ok=allowed;q.io.output_ready=output_ready;
 q.count=2;q.steps[0].voltage_mv=5000;q.steps[0].time_ms=100;q.steps[0].slew_mv_per_s=1000;q.steps[0].output_action=PSU_STEP_ON;
 q.steps[1]=q.steps[0];q.steps[1].voltage_mv=0;q.steps[1].output_action=PSU_STEP_KEEP;
 assert(psu_seq_start(&q,0));unsigned last=0,down=0;
 for(t=0;t<11000;t+=10){
  unsigned before=q.step_index;psu_seq_tick(&q,t);
  if(before==0&&q.step_index==0){assert(q.ramp_mv>=last);assert(q.ramp_mv-last<=130);last=q.ramp_mv;}
  if(q.step_index==1){if(!down){assert(q.ramp_mv==5000);down=1;last=5000;}else{assert(q.ramp_mv<=last);assert(last-q.ramp_mv<=130);last=q.ramp_mv;}}
 }
 assert(down&&q.run==PSU_SEQ_DONE&&q.ramp_mv==0);
 puts("PASS bidirectional 1 V/s: 0 -> 5 V -> 0, no reset/jump at second step, target reached before dwell completes");
 psu_chg_init(&c);c.profile.cells=4;c.profile.confirmed=c.profile.polarity_checked=1;
 assert(psu_chg_set_target(&c.profile,16400));assert(psu_chg_target_mv(&c.profile)==16400);
 assert(!psu_chg_set_target(&c.profile,17000));assert(psu_chg_target_mv(&c.profile)==16400);
 c.io.limits=limits;c.io.output=output;s.pack_mv=0;s.start_allowed=s.telemetry_ok=1;s.permit=s.limits_applied=s.output_ready=0;
 assert(psu_chg_start(&c,&c.profile,&s,20000));psu_chg_tick(&c,&s,20010);
 assert(c.state==CHG_STARTING&&mv==16400&&ma==100);
 s.limits_applied=1;psu_chg_tick(&c,&s,20020);assert(c.output_started&&c.running);
 s.output_ready=s.permit=1;s.pack_mv=13000;psu_chg_tick(&c,&s,20030);assert(c.state==CHG_CC);
 puts("PASS 4S reduced target 16.4 V; 17 V rejected; zero OFF measurement does not deadlock current-limited start");
 return 0;
}
