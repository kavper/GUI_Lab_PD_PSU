#include "psu_app.h"
#if defined(SIMULATOR) || defined(PSU_SIMULATOR)
#include <string.h>
static void w16(uint8_t *p,uint16_t v){p[0]=v;p[1]=v>>8;}
static void w32(uint8_t *p,uint32_t v){w16(p,v);w16(p+2,v>>16);}
static uint32_t r32(uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void emit(uint8_t t,uint8_t seq,uint8_t *p,unsigned n,uint32_t now){uint8_t f[120];size_t len=g4_frame(f,t,seq,p,n);g4_rx_bytes(psu_g4(),f,len,now);}
void psu_sim_tick(uint32_t now){static uint8_t init,on,remote,permit=1,role=1,seq,asleep;static uint32_t mv=12000,ma=1500,slow;uint8_t f[120],d[72];int n;unsigned i;
 if(!init){init=1;psu_app_set_g4_uart(1);}g4_process(psu_g4(),now);
 n=g4_pop_frame(psu_g4(),f,sizeof(f),now);if(n){uint8_t type=f[3];if(type==0x21&&f[2]==14&&!memcmp(f+5,"BMS SHUTDOWN",12)){asleep=1;on=remote=0;}if(type==1){mv=r32(f+5);ma=r32(f+9);}if(type==2){on=1;permit=1;}if(type==3){on=remote=0;}if(type==8){permit=f[5];if(!permit)on=0;}if(type==9)remote=f[5]&&on;if(type==11)role=f[5];d[0]=type;if(type!=7)emit(0x81,f[4],d,1,now);}
 memset(d,0,sizeof(d));w32(d,20000);w32(d+4,on?mv+1500:0);w32(d+8,on?410:0);w32(d+16,mv);w32(d+20,ma);w32(d+24,on?mv:0);w32(d+28,on?410:0);w32(d+32,on?mv+1500:0);w32(d+36,mv);w32(d+40,ma);w32(d+44,on?mv+1500:0);w32(d+48,on?mv+1500:0);w16(d+52,4);w16(d+54,on?410:0);d[66]=(on?0xc3:8)|(permit?32:0);d[67]=64|(on?3:0)|(remote?8:0);d[68]=on?9:0;d[69]=on?1:0;emit(0x10,seq++,d,72,now);
 if(slow&&now-slow<200){return;}
 slow=now;
 memset(d,0,sizeof(d));d[0]=d[1]=1;d[2]=4;d[13]=5;d[16]=12;w16(d+18,0x17);d[22]=d[23]=d[24]=asleep?0:1;d[25]=4;for(i=0;i<5;i++)w16(d+26+i*2,i==3?0xffff:4001+i);w16(d+36,4001);w16(d+38,4005);w16(d+40,4);w16(d+42,16011);w16(d+44,16020);w16(d+46,16011);w32(d+48,320);d[52]=1;w32(d+56,2450);w16(d+64,780);w16(d+66,315);w16(d+68,2982);d[70]=2;d[71]=31;emit(0x11,seq++,d,72,now);
 memset(d,0,sizeof(d));d[0]=1;d[1]=0x85|(role==2?8:0);w16(d+2,0x8400);d[5]=2;d[7]=role;d[8]=1;d[9]=0x42;d[10]=role;w32(d+12,16020);w32(d+16,16400);w32(d+20,320);w32(d+24,352);w32(d+32,19980);w32(d+36,600);w32(d+40,16800);w32(d+44,1500);w32(d+48,3000);w32(d+52,20010);w32(d+56,20000);w32(d+60,3000);emit(0x12,seq++,d,64,now);
 memset(d,0,sizeof(d));w32(d,1500);w32(d+4,350);for(i=0;i<4;i++)w16(d+8+i*2,253+i*10);d[16]=on?35:0;d[17]=1;d[18]=!on;d[19]=1;w16(d+20,on?mv:0);w16(d+22,on?mv-40:0);w16(d+24,on?20:0);d[26]=on?0:1;d[27]=remote?3:0;w16(d+28,on?1600:0);emit(0x13,seq++,d,32,now);
}
#endif
