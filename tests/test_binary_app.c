#include "psu_app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void frame(uint8_t type,uint8_t seq,uint8_t *p,unsigned len,unsigned ms){uint8_t f[120];size_t n=g4_frame(f,type,seq,p,len);g4_rx_bytes(psu_g4(),f,n,ms);}
int main(void){unsigned ms;PsuSnapshot s;uint8_t p[72]={0},f[120],ack[2];int n;
 psu_app_init();for(ms=0;ms<400;ms+=16){psu_sim_tick(ms);psu_app_tick(ms);}psu_snapshot(&s);assert(s.g4_link==G4_LINK_ONLINE&&s.bms_valid&&s.temperature_valid);assert(s.mos_centi==2530&&!s.output_requested);assert(s.battery_energy_uwms>0);
 assert(psu_app_set_output(1,PSU_SRC_LCD));for(;ms<600;ms+=16){psu_sim_tick(ms);psu_app_tick(ms);}psu_snapshot(&s);assert(s.output_confirmed&&s.output_requested);
 psu_app_tick(ms+210);psu_snapshot(&s);assert(!s.current_valid&&s.output_requested&&!s.fault_latched);
 for(ms+=230;ms<1000;ms+=16){psu_sim_tick(ms);psu_app_tick(ms);}psu_snapshot(&s);assert(s.output_requested&&s.output_confirmed&&!s.fault_latched);assert(psu_app_shutdown());for(;ms<1100;ms+=16){psu_sim_tick(ms);psu_app_tick(ms);}psu_snapshot(&s);assert(!s.fault_latched);
 psu_app_tick(ms+1100);psu_snapshot(&s);assert(!s.bms_valid&&!s.temperature_valid&&!s.battery_energy_valid);
 psu_app_init();psu_app_set_g4_uart(1);p[52]=4;p[67]=64;p[16]=0x88;p[17]=0x13;p[36]=0x88;p[37]=0x13;frame(0x10,1,p,72,10);psu_app_tick(10);g4_pop_frame(psu_g4(),f,120,10);ack[0]=3;frame(0x81,f[4],ack,1,10);psu_app_tick(10);
 assert(psu_app_set_limits(5000,0,PSU_SRC_LCD));n=g4_pop_frame(psu_g4(),f,120,11);assert(n==15&&f[3]==1);psu_app_tick(11);psu_snapshot(&s);assert(s.last_cmd_state==PSU_CMD_SENT);frame(0x10,2,p,72,12);psu_app_tick(12);psu_snapshot(&s);assert(s.last_cmd_state!=PSU_CMD_ACK);ack[0]=1;frame(0x81,f[4],ack,1,13);psu_app_tick(13);psu_snapshot(&s);assert(s.last_cmd_state==PSU_CMD_ACK);
 assert(psu_app_power_shutdown());n=g4_pop_frame(psu_g4(),f,120,14);assert(n==7&&f[3]==3);n=g4_pop_frame(psu_g4(),f,120,15);assert(n==19&&f[3]==0x21&&!memcmp(f+5,"BMS SHUTDOWN",12));assert(!psu_app_set_output(1,PSU_SRC_LCD));
 ack[0]=0x21;ack[1]=4;frame(0x82,f[4],ack,2,16);psu_app_tick(16);psu_snapshot(&s);
 assert(!s.power_shutdown_requested&&!s.output_requested&&strstr(s.command_error,"POWER OFF REJECTED"));
 puts("PASS: binary simulator, temperature, battery energy, stale METER without OFF, recovery preserves ON, explicit shutdown, independent stale panels, SET requires ACK, full BMS shutdown");return 0;
}
