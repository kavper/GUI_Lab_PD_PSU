#include "psu_charger.h"
#include "psu_seq.h"
#include <assert.h>
#include <stdio.h>
static unsigned mv,ma,on_calls,off_calls,permit_calls,ready,applied;
static void limits(uint32_t v,uint32_t i,void*u){(void)u;mv=v;ma=i;}
static void output(int on,void*u){(void)u;if(on){assert(mv>0);on_calls++;}else off_calls++;}
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
 psu_chg_tick(&c,&s,2);assert(c.state==CHG_STARTING&&mv==4200&&ma==100&&!on_calls&&!permit_calls);
 psu_chg_tick(&c,&s,50);assert(!on_calls);
 s.limits_applied=1;psu_chg_tick(&c,&s,100);assert(on_calls==1&&c.state==CHG_STARTING);
 psu_chg_tick(&c,&s,1000);assert(c.running&&c.state==CHG_STARTING&&!permit_calls);
 s.permit=s.output_ready=1;psu_chg_tick(&c,&s,1100);assert(c.state==CHG_PRECHARGE);
 s.permit=0;psu_chg_tick(&c,&s,1200);assert(!c.running&&c.state==CHG_FAULT&&off_calls==1);
 s.permit=1;psu_chg_tick(&c,&s,1300);assert(!c.running&&on_calls==1);
 s.start_allowed=s.permit=0;assert(!psu_chg_start(&c,&c.profile,&s,1400));
 s.start_allowed=1;s.output_ready=s.limits_applied=0;assert(psu_chg_start(&c,&c.profile,&s,1500));
 psu_chg_tick(&c,&s,1510);psu_chg_tick(&c,&s,6520);assert(!c.running&&c.state==CHG_FAULT);
 puts("PASS charger: no initial PERMIT required; SET before ON; delayed G4/G0 readiness; no forced PERMIT; running loss trips; timeout and no automatic restart");
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
 return 0;
}
