#include "g4_ascii.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static G4Port port;
static void put(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static void receive(uint8_t *p,unsigned n,uint8_t seq,uint32_t now){uint8_t frame[120];size_t size=g4_frame(frame,0x14,seq,p,n);g4_rx_bytes(&port,frame,size,now);}
int main(void){
 uint8_t p[44]={1,7,1,7};put(p+4,0x1234);put(p+8,(5u<<28)|(300u<<10)|300u);put(p+12,(400u<<10)|300u);
 for(unsigned i=0;i<7;i++)put(p+16+4*i,((100u+60u*i)<<10)|300u);
 g4_init(&port);uint8_t frame[120];size_t n=g4_frame(frame,0x14,7,p,sizeof(p));
 for(size_t i=0;i<n;i++)g4_rx_bytes(&port,frame+i,1,100+(uint32_t)i);
 G4PdCapabilities c;g4_pd_capabilities_snapshot(&port,&c);
 assert(c.valid&&c.count==7&&c.flags==7&&c.role==1&&c.attach_generation==0x1234&&c.rdo==((5u<<28)|(300u<<10)|300u));
 assert(c.pdo[6]==((460u<<10)|300u));uint32_t stamp=c.ms;
 receive(p,sizeof(p),7,500);g4_pd_capabilities_snapshot(&port,&c);assert(c.ms==stamp); /* duplicate cannot refresh freshness */
 unsigned errors=port.parse_error_count;p[3]=8;receive(p,sizeof(p),8,501);assert(port.parse_error_count==++errors);p[3]=7;
 p[0]=2;receive(p,sizeof(p),9,502);assert(port.parse_error_count==++errors);p[0]=1;
 p[1]=6;receive(p,sizeof(p),10,503);assert(port.parse_error_count==++errors);p[1]=7;
 put(p+8,0x80000000u);receive(p,sizeof(p),11,504);assert(port.parse_error_count==++errors);put(p+8,(5u<<28)|(300u<<10)|300u);
 receive(p,43,12,505);assert(port.parse_error_count==++errors);
 n=g4_frame(frame,0x14,13,p,sizeof(p));frame[15]^=1;g4_rx_bytes(&port,frame,n,506);assert(port.parse_error_count==++errors);
 /* A normal TC frame exposes the attach epoch without moving existing keys. */
 uint8_t tc[64]={0};tc[1]=128;tc[7]=tc[10]=1;tc[11]=0x34;n=g4_frame(frame,0x12,14,tc,sizeof(tc));g4_rx_bytes(&port,frame,n,510);
 G4Record record;int64_t value;g4_record_snapshot(&port,G4_RECORD_TC,&record);assert(g4_record_value(&record,"attach_gen",&value)&&value==0x34);
 /* New source session and explicit detach, never a previous partner's list. */
 p[1]=15;p[2]=2;put(p+4,0x1235);receive(p,sizeof(p),15,600);g4_pd_capabilities_snapshot(&port,&c);assert(c.flags==15&&c.role==2&&c.attach_generation==0x1235);
 memset(p,0,sizeof(p));p[0]=1;put(p+4,0x1236);receive(p,sizeof(p),16,700);g4_pd_capabilities_snapshot(&port,&c);assert(c.flags==0&&c.count==0&&!c.rdo);
 puts("PASS PD capability frame: fragmented CRC, duplicate freshness, payload validation, current session and detach");return 0;
}
