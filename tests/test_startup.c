#include "psu_app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t meter_seq;
static uint8_t wire[120];
static PsuSnapshot snapshot(void) { PsuSnapshot s; psu_snapshot(&s); return s; }
static void put32(uint8_t *p, uint32_t v) {
  p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static void receive(uint8_t type, uint8_t seq, const uint8_t *p, size_t n, uint32_t ms) {
  uint8_t f[120];size_t len=g4_frame(f,type,seq,p,n);g4_rx_bytes(psu_g4(),f,len,ms);
}
static void meter(uint32_t ms, uint32_t fault, uint8_t ctrl, uint8_t flags,
                  int fresh, uint32_t g4_fault, int latch) {
  uint8_t p[72]={0};
  put32(p+16,12000);put32(p+20,2000);put32(p+36,12000);put32(p+40,2000);
  p[52]=4;put32(p+58,fault);put32(p+62,g4_fault);
  p[66]=flags;p[67]=(fresh?64:0)|(latch?128:0);p[68]=ctrl;
  receive(0x10,++meter_seq,p,sizeof(p),ms);
}
static void tick_meter(uint32_t ms,uint32_t fault,uint8_t ctrl,uint8_t flags,int fresh) {
  meter(ms,fault,ctrl,flags,fresh,0,0);psu_app_tick(ms);
}
static uint8_t transmit(uint32_t ms, uint8_t type) {
  int n;
  do {
    n=g4_pop_frame(psu_g4(),wire,sizeof(wire),ms);assert(n>0);
    if(wire[3]==5){uint8_t ping=5;receive(0x81,wire[4],&ping,1,ms);}
  } while(wire[3]==5);
  assert(wire[3]==type);
  psu_app_tick(ms);return wire[4];
}
static void ack(uint32_t ms,uint8_t type,uint8_t seq) {
  receive(0x81,seq,&type,1,ms);psu_app_tick(ms);
}
static void reset_off(uint32_t fault) {
  psu_app_init();psu_app_set_g4_uart(1);meter_seq=0;
  tick_meter(10,fault,0,4,1); /* OFF: POWER_KILL asserted, no PERMIT */
  ack(10,3,transmit(10,3));assert(!snapshot().fault_latched);
}
static void start_running(void) {
  reset_off(G4_G0_POWER_KILL);
  assert(psu_app_set_output(1,PSU_SRC_LCD));
  uint8_t seq=transmit(11,2);
  tick_meter(12,0,9,0xe3,1);ack(12,2,seq);
  assert(snapshot().output_requested&&snapshot().output_confirmed);
  assert(snapshot().output_phase==G4_OUTPUT_RUNNING);
}
static void assert_off_latched(const char *why,uint32_t ms) {
  PsuSnapshot s=snapshot();assert(s.fault_latched&&!s.output_requested);
  assert(strstr(s.fault,why));transmit(ms,3);
}
static void clear(uint32_t ms) {
  assert(psu_app_clear_fault());ack(ms,4,transmit(ms,4));
}
int main(void) {
  reset_off(G4_G0_MEAS_LOST|G4_G0_POWER_KILL);
  assert(!psu_app_set_output(1,PSU_SRC_LCD));
  assert(strstr(snapshot().last_on_reject,"VALID MEASUREMENTS"));
  assert(!snapshot().fault_latched);
  tick_meter(20,G4_G0_POWER_KILL,0,4,1);
  assert(psu_app_set_output(1,PSU_SRC_LCD));transmit(20,2);
  puts("PASS: OFF MEAS_LOST recovers; manual ON needs fresh valid measurements");

  start_running();
  /* Lost then healthy in one RX batch still trips the running fault. */
  meter(20,G4_G0_MEAS_LOST,9,0xe3,1,0,0);meter(20,0,9,0xe3,1,0,0);
  psu_app_tick(20);assert(!snapshot().fault_latched);
  meter(20,G4_G0_MEAS_LOST,12,4,1,0,1);psu_app_tick(20);
  assert_off_latched("MEAS_LOST",20);
  tick_meter(25,0,0,0,1);assert(!snapshot().output_requested);
  assert(!psu_app_set_output(1,PSU_SRC_LCD));
  clear(26);assert(snapshot().fault_latched);
  tick_meter(27,G4_G0_MEAS_LOST,0,4,1);assert(snapshot().fault_latched);
  tick_meter(28,0,0,0,1);assert(!snapshot().fault_latched&&!snapshot().output_requested);
  assert(psu_app_set_output(1,PSU_SRC_LCD));
  puts("PASS: G4-confirmed MEAS_LOST stops UI consumers; CLEAR requires healthy data; no auto ON");

  start_running();tick_meter(20,G4_G0_POWER_KILL,9,0xe6,1);
  assert(!snapshot().fault_latched);
  meter(20,G4_G0_POWER_KILL,12,4,1,0,1);psu_app_tick(20);
  assert_off_latched("POWER_KILL",20);
  tick_meter(21,0,0,0,1);assert(!snapshot().output_requested);
  puts("PASS: G4-confirmed running POWER_KILL is an emergency");
  start_running();tick_meter(20,0,9,0xe7,1);
  assert(!snapshot().fault_latched); /* G4 owns interpretation of its kill pin. */
  meter(21,0,12,4,1,0,1);psu_app_tick(21);
  assert_off_latched("CONTROL FAULT",21);
  puts("PASS: raw kill does not create a third latch; G4 supervisor fault is authoritative");

  reset_off(G4_G0_POWER_KILL|G4_G0_VIN_LOW);
  assert(psu_app_set_output(1,PSU_SRC_LCD));uint8_t seq=transmit(11,2);
  /* Mirror G4: DCDC start -> WAIT_LINK -> WAIT_PERMIT -> WAIT_VIN ->
     SET -> wait ACK -> OUT ON -> RUNNING. Delayed G0 during early phases. */
  for(uint32_t ms=20;ms<700;ms+=20) {
    tick_meter(ms,G4_G0_MEAS_LOST|G4_G0_POWER_KILL|G4_G0_VIN_LOW,
               ms<100?1:2,0x46,ms<100?0:1);
    assert(!snapshot().fault_latched&&snapshot().output_requested);
    if(g4_pop_frame(psu_g4(),wire,sizeof(wire),ms)) {
      assert(wire[3]==5);ack(ms,5,wire[4]); /* heartbeat, never OFF/PERMIT */
    }
  }
  tick_meter(700,0,3,0xe2,1);
  for(uint8_t ctrl=4;ctrl<=8;ctrl++)tick_meter(700+ctrl,0,ctrl,0xe2,1);
  tick_meter(720,0,9,0xe3,1);ack(720,2,seq);
  assert(snapshot().output_confirmed&&!snapshot().fault_latched);
  puts("PASS: complete delayed G0 start; no automatic OFF or forced PERMIT");

  reset_off(0);assert(psu_app_set_output(1,PSU_SRC_LCD));transmit(11,2);
  for(uint8_t ctrl=4;ctrl<=8;ctrl++) {
    tick_meter(20+ctrl,0,ctrl,0x26,1); /* delayed raw kill; mask itself is zero */
    assert(!snapshot().fault_latched&&snapshot().output_requested);
    assert(!g4_pop_frame(psu_g4(),wire,sizeof(wire),20+ctrl));
  }
  tick_meter(30,0,2,0x46,1); /* G4 handles retry back to WAIT_PERMIT */
  tick_meter(31,0,7,0xe2,1);tick_meter(32,0,9,0xe3,1);
  assert(!snapshot().fault_latched&&snapshot().output_confirmed);
  puts("PASS: late startup POWER_KILL with G0 fault=0 allows G4 PERMIT retry");

  reset_off(0);assert(psu_app_set_output(1,PSU_SRC_LCD));transmit(11,2);
  tick_meter(20,G4_G0_MEAS_LOST,4,0xe2,1);assert(!snapshot().fault_latched);
  meter(20,G4_G0_MEAS_LOST,12,4,1,0,1);psu_app_tick(20);
  assert_off_latched("MEAS_LOST",20);
  assert(strstr(snapshot().fault_context,"ctrl=12"));
  puts("PASS: late-start measurement fault still trips and records originating frame");

  start_running();assert(psu_app_shutdown());transmit(20,3);
  /* G4 drops PERMIT before G0 updates OUT. The first stopped-path
     METER can legitimately retain out/want/RUNNING with raw kill=1. */
  tick_meter(20,0,9,0xe7,1);
  assert(!snapshot().fault_latched);
  assert(snapshot().output_phase==G4_OUTPUT_STOPPING);
  tick_meter(21,0,8,0x26,1);assert(!snapshot().fault_latched);
  assert(snapshot().output_phase==G4_OUTPUT_STOPPING);
  assert(!psu_app_set_output(1,PSU_SRC_LCD));
  tick_meter(22,0,0,4,1);assert(!snapshot().fault_latched);
  assert(snapshot().output_phase==G4_OUTPUT_IDLE);
  assert(psu_app_set_output(1,PSU_SRC_LCD));
  puts("PASS: explicit OFF permits delayed want/ctrl/kill convergence; ON waits for stopped state");

  start_running();assert(psu_app_shutdown());transmit(20,3);
  tick_meter(21,G4_G0_MEAS_LOST,9,0xe7,1);assert(!snapshot().fault_latched);
  meter(21,G4_G0_MEAS_LOST,12,4,1,0,1);psu_app_tick(21);
  assert_off_latched("MEAS_LOST",21);
  puts("PASS: expected OFF kill does not trip H7; confirmed G4 measurement fault remains actionable");

  start_running();psu_app_tick(63);
  assert(!snapshot().fault_latched && snapshot().output_requested);
  psu_app_tick(600);
  assert(!snapshot().fault_latched && snapshot().output_requested);
  while(g4_pop_frame(psu_g4(),wire,sizeof(wire),600))assert(wire[3]!=3);
  tick_meter(601,0,9,0xe3,1);
  assert(!snapshot().fault_latched && snapshot().output_requested);
  reset_off(0);psu_app_tick(211);assert(!psu_app_set_output(1,PSU_SRC_LCD));
  assert(strstr(snapshot().last_on_reject,"METER STALE"));
  reset_off(0);tick_meter(20,0,0,0,0);assert(!psu_app_set_output(1,PSU_SRC_LCD));
  assert(strstr(snapshot().last_on_reject,"FRESH G0"));
  puts("PASS: stale METER does not queue OFF or latch H7; fresh data resumes and stale idle ON is blocked");

  start_running();meter(20,0,9,0xe3,0,0,0);meter(20,0,9,0xe3,1,0,0);
  psu_app_tick(20);assert(!snapshot().fault_latched);
  meter(20,0,12,4,0,0,1);psu_app_tick(20);assert_off_latched("CONTROL FAULT",20);
  puts("PASS: G0 freshness transition does not invent H7 trip; confirmed G4 fault stops output");

  start_running();assert(psu_seq_start(psu_sequencer(),13));
  meter(20,G4_G0_MEAS_LOST,12,4,1,0,1);psu_app_tick(20);
  assert(snapshot().fault_latched&&psu_sequencer()->run!=PSU_SEQ_RUN);
  /* Shutdown may queue more than one OFF while stopping consumers. */
  while(g4_pop_frame(psu_g4(),wire,sizeof(wire),20))assert(wire[3]==3);
  tick_meter(21,0,0,0,1);clear(22);tick_meter(23,0,0,0,1);
  assert(!snapshot().fault_latched&&!snapshot().output_requested);
  assert(psu_sequencer()->run!=PSU_SEQ_RUN);
  puts("PASS: emergency stops sequencer; CLEAR never resumes it");

  reset_off(0);meter(20,0,0,0,1,4,0);psu_app_tick(20);
  assert_off_latched("G4 FAULT",20);clear(21);
  meter(22,0,0,0,1,4,0);psu_app_tick(22);assert(snapshot().fault_latched);
  tick_meter(23,0,0,0,1);assert(snapshot().fault_latched); /* new fault invalidated CLEAR */
  clear(24);tick_meter(25,0,0,0,1);assert(!snapshot().fault_latched);
  reset_off(0);tick_meter(20,1,0,0,1);assert(!snapshot().fault_latched);
  assert(!psu_app_set_output(1,PSU_SRC_LCD)); /* active HW_INIT blocks readiness */
  reset_off(0);meter(20,0,0,0,1,0,1);psu_app_tick(20);assert_off_latched("CONTROL FAULT",20);
  puts("PASS: real G4/G0/control faults stay latched; CLEAR cannot bypass active fault");

  reset_off(0);meter(20,0,12,4,1,0,1);psu_app_tick(20);
  assert_off_latched("CONTROL FAULT",20);clear(21);
  meter(22,0,12,4,1,0,1);psu_app_tick(22); /* buffered pre-CLEAR frame */
  assert(snapshot().fault_latched);
  tick_meter(23,0,10,4,1);tick_meter(24,0,11,4,1);tick_meter(25,0,0,4,1);
  assert(!snapshot().fault_latched && !snapshot().output_requested);
  assert(psu_app_set_output(1,PSU_SRC_LCD));
  puts("PASS: CLEAR survives buffered ctrl=FAULT and completes G4 OFF recovery without auto ON");

  reset_off(0);assert(psu_app_set_output(1,PSU_SRC_LCD));seq=transmit(11,2);
  uint8_t nack[]={2,4};receive(0x82,seq,nack,2,12);psu_app_tick(12);
  assert_off_latched("REJECTED",12);PsuSnapshot s=snapshot();
  assert(s.nack_valid&&s.nack_type==2&&s.nack_seq==seq&&s.nack_reason==4&&s.nack_matched);
  uint8_t unmatched[]={1,5};receive(0x82,seq,unmatched,2,13);psu_app_tick(13);
  assert(snapshot().nack_reason==5&&!snapshot().nack_matched);
  puts("PASS: received NACK TYPE/SEQ/reason and correlation remain diagnostic");

  reset_off(0);assert(psu_app_set_output(1,PSU_SRC_LCD));transmit(11,2);
  for(uint32_t ms=20;ms<=10040;ms+=20)tick_meter(ms,G4_G0_POWER_KILL,2,0x46,1);
  assert(snapshot().fault_latched&&!snapshot().output_requested);
  puts("PASS: startup wait remains bounded by 10-second host timeout after G4's 8-second deadline");
  return 0;
}
