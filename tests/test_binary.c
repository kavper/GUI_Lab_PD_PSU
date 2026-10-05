#include "g4_ascii.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void send(G4Port *p,uint8_t type,uint8_t seq,uint8_t *d,unsigned n,unsigned now){uint8_t f[120];size_t len=g4_frame(f,type,seq,d,n);g4_rx_bytes(p,f,len,now);}
int main(void){G4Port p;uint8_t f[120],d[72]={0},a[2];int n;uint16_t id;uint8_t state;unsigned i;int64_t v;
 const uint8_t expected[]={0xa5,0x5a,0x0a,1,7,0x88,0x13,0,0,0x64,0,0,0,0x5d,0x97};
 g4_init(&p);d[0]=0x88;d[1]=0x13;d[4]=100;n=g4_frame(f,1,7,d,8);assert(n==15&&!memcmp(f,expected,15));
 memset(d,0,sizeof(d));d[24]=0x74;d[25]=0x13;d[4]=0x64;d[5]=0x19;d[66]=0xe3;d[67]=0x43;d[52]=4;d[68]=9;d[69]=1;
 n=g4_frame(f,0x10,3,d,72);for(i=0;i<(unsigned)n;i++)g4_rx_bytes(&p,f+i,1,10);assert(p.telemetry.vout_mv==4980&&p.telemetry.rail_mv==6500&&p.telemetry.out);
 f[n-1]^=1;g4_rx_bytes(&p,f,n,20);assert(p.telemetry.ms==10&&p.parse_error_count==1);f[n-1]^=1;g4_rx_bytes(&p,f,n,30);assert(p.telemetry.ms==10);send(&p,0x10,4,d,72,40);assert(p.telemetry.ms==40);g4_process(&p,91);assert(p.link==G4_LINK_STALE);
 assert(g4_set_limits(&p,5000,100,10));n=g4_pop_frame(&p,f,120,100);assert(n==15&&f[3]==1);a[0]=1;send(&p,0x81,f[4]+1,a,1,110);assert(!g4_response(&p,&id,&state));
 assert(g4_set_limits(&p,6000,200,11));assert(g4_set_limits(&p,7000,300,12));assert(g4_response(&p,&id,&state)&&id==11&&state==3);assert(!g4_pop_frame(&p,f,120,120));
 assert(g4_off(&p,13));n=g4_pop_frame(&p,f,120,120);assert(n==7&&f[3]==3);a[0]=3;send(&p,0x81,f[4],a,1,121);assert(g4_response(&p,&id,&state)&&id==13&&state==2);
 a[0]=1;send(&p,0x81,p.inflight_seq[0],a,1,125);assert(g4_response(&p,&id,&state)&&id==10&&state==2);n=g4_pop_frame(&p,f,120,130);assert(n==15&&f[5]==0x58&&f[6]==0x1b);g4_process(&p,931);assert(g4_response(&p,&id,&state)&&id==12&&state==4);
 assert(!g4_set_limits(&p,27001,100,20));assert(!g4_set_limits(&p,100,5001,20));
 assert(g4_on(&p,21));g4_pop_frame(&p,f,120,1000);assert(!g4_on(&p,22));g4_process(&p,1801);assert(p.inflight_len[1]);g4_process(&p,11001);assert(g4_response(&p,&id,&state)&&id==21&&state==4);
 memset(d,0,sizeof(d));d[0]=d[1]=d[52]=1;d[25]=4;d[32]=d[33]=0xff;d[64]=d[65]=0xff;send(&p,0x11,1,d,72,10000);assert(p.battery.cell_mv[3]==-1&&!g4_record_value(&p.records[1],"soc_permille",&v));
 memset(d,0,sizeof(d));d[8]=0;d[9]=0x80;d[19]=1;d[20]=d[21]=d[28]=d[29]=0xff;send(&p,0x13,1,d,32,10000);assert(!g4_record_value(&p.records[3],"t1",&v));assert(!g4_record_value(&p.records[3],"local_mv",&v));assert(!g4_record_value(&p.records[3],"fan_rpm",&v));
 g4_init(&p);assert(g4_usb_role(&p,"SINK",1));{uint8_t reserved=p.wire_queue[p.tail][4];for(i=0;i<600;i++)assert(g4_set_limits(&p,5000+i,100,0));n=g4_pop_frame(&p,f,120,1);assert(n==15&&f[4]!=reserved);n=g4_pop_frame(&p,f,120,2);assert(n==8&&f[4]==reserved&&f[3]==11);}
 g4_init(&p);assert(g4_on(&p,1));assert(g4_off(&p,2));assert(g4_off(&p,3));assert(g4_response(&p,&id,&state)&&id==1&&state==3);n=g4_pop_frame(&p,f,120,1);assert(n==7&&f[3]==3);n=g4_pop_frame(&p,f,120,2);assert(n==7&&f[3]==3);assert(!g4_pop_frame(&p,f,120,3));
 g4_init(&p);memset(d,0,sizeof(d));n=g4_frame(f,0x10,1,d,72);g4_rx_bytes(&p,f,12,1);g4_rx_bytes(&p,f,n,20);assert(p.telemetry.valid);
 g4_init(&p);memset(d,0,sizeof(d));send(&p,0x10,1,d,72,10);
 g4_process(&p,100);n=g4_pop_frame(&p,f,120,100);assert(n==7&&f[3]==5);
 a[0]=5;send(&p,0x81,f[4],a,1,101);assert(!g4_response(&p,&id,&state));
 g4_process(&p,200);assert(g4_off(&p,55));n=g4_pop_frame(&p,f,120,200);assert(f[3]==3);
 n=g4_pop_frame(&p,f,120,200);assert(n==7&&f[3]==5);
 puts("PASS: heartbeat uses binary PING without user transactions; OFF retains priority");
 puts("PASS: CRC vectors, fragmented RX, CRC rejection, duplicate freshness, stale, SET latest-wins, ACK matching, OFF priority/FIFO, timeouts, sentinel values, sequence wrap and truncated-frame recovery");return 0;
}
