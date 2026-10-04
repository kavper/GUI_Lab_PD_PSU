#include "g4_ascii.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Protect queue updates shared by the UI and communications task. */
static uint32_t lock(void) {
#if defined(__arm__) || defined(__thumb__)
  uint32_t p; __asm volatile("mrs %0, primask\ncpsid i" : "=r"(p) :: "memory"); return p;
#else
  return 0;
#endif
}
static void unlock(uint32_t p) {
#if defined(__arm__) || defined(__thumb__)
  __asm volatile("msr primask, %0" :: "r"(p) : "memory");
#else
  (void)p;
#endif
}
static void log_line(G4Port *p,int tx,const char *s,uint32_t ms) {
  G4RawLine *l=&p->log[p->log_pos];
  snprintf(l->text,sizeof(l->text),"%s",s);l->dir_tx=(uint8_t)tx;l->ms=ms;
  p->log_pos=(p->log_pos+1)%G4_RAW_LOG;if(p->log_count<G4_RAW_LOG)++p->log_count;
}
static const char *const keys_t[] = {
  "vin_mv","vout_mv","iout_ma","i_buck_ma","i_boost_ma","set_mv","ilim_ma",
  "duty_a_x10","duty_c_x10","ucc_a","ucc_c","run","fault","pd","pd_mv","pd_ma","pd_mw",
  "permit","rem_sense","g0","g0_out","g0_want","g0_ctrl","g0_kill","g0_outoff","g0_fault",
  "g0_vout_mv","g0_iout_ma","vpre_req_mv","vpre_cmd_mv","reg_ok","stage_en","ps_en","flt",
  "hold_ms","ps_err","g0_rx","g0_tlm","g0_age_ms","g0_err","g0_uart","pm_st","fmt","mode", NULL
};
static const char *const keys_tb[] = {
  "bms","cfg","st","fault","alert","alarm","sa","sb","sc","fet","manuf","init_step",
  "vcell_rb","batt","cfg_fail","chg","dsg","fets","series","c1_mv","c2_mv","c3_mv","c4_mv","c5_mv",
  "min_mv","max_mv","dV_mv","sum_mv","pack_mv","stack_mv","i_pack_ma","i_cc2_ma","sample","alerts","i2c_err","passq_mah","soc_permille","cc1_ma","int_temp_dK","balance_mask","soc_flags", NULL
};
static const char *const keys_tc[] = {
  "bq_ok","bq_vbat_mv","bq_vsys_mv","bq_ibat_ma","bq_ichg_ma","bq_idchg_ma","bq_vbus_mv","bq_iin_ma",
  "bq_vreg_mv","bq_ichg_set_ma","bq_iin_set_ma","bq_st","bq_fault","bq_in","bq_pre","bq_fast","bq_otg",
  "bq_iindpm","bq_vindpm","tps_vbus_mv","cc1","cc2","role","conn","plug","typec","rst","rst_busy",
  "pd_role","pd_mv","pd_ma", NULL
};
static const char *const keys_aux[]={"dac_cv_mv","dac_cc_mv","t1","t2","t3","t4","fan","pgood","bleed","valid","local_mv","remote_p_mv","remote_n_mv","sense_code","sense_flags","fan_rpm",NULL};
static const char *const *record_keys(uint8_t kind) {
  return kind==G4_RECORD_AUX ? keys_aux : kind==G4_RECORD_T ? keys_t : kind==G4_RECORD_TB ? keys_tb : keys_tc;
}
int g4_record_value(const G4Record *record,const char *key,int64_t *out) {
  unsigned i;const char *const *keys;
  if(!record||!record->valid||record->kind>G4_RECORD_AUX||!key||!out)return 0;
  keys=record_keys(record->kind);
  for(i=0;keys[i] && i<64;i++)if(!strcmp(keys[i],key)) {
    if(!(record->present&((uint64_t)1<<i)))return 0;
    *out=record->values[i];return 1;
  }
  return 0;
}
void g4_record_snapshot(const G4Port *p,uint8_t kind,G4Record *out) {
  uint32_t irq;if(!p||!out||kind>G4_RECORD_AUX)return;
  irq=lock();*out=p->records[kind];unlock(irq);
}
static uint16_t u16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t u32(const uint8_t *p){return (uint32_t)u16(p)|((uint32_t)u16(p+2)<<16);}
static void put32(uint8_t *p,uint32_t v){p[0]=v;p[1]=v>>8;p[2]=v>>16;p[3]=v>>24;}
uint16_t g4_crc16(const uint8_t *d,size_t n){uint16_t c=0xffff;size_t i;unsigned b;for(i=0;i<n;i++){c^=(uint16_t)d[i]<<8;for(b=0;b<8;b++)c=(c&0x8000)?(uint16_t)((c<<1)^0x1021):(uint16_t)(c<<1);}return c;}
size_t g4_frame(uint8_t *o,uint8_t t,uint8_t seq,const uint8_t *d,size_t n){uint16_t c;if(n>113)return 0;o[0]=0xa5;o[1]=0x5a;o[2]=(uint8_t)(n+2);o[3]=t;o[4]=seq;if(n)memcpy(o+5,d,n);c=g4_crc16(o+2,n+3);o[n+5]=c;o[n+6]=c>>8;return n+7;}
static void field(G4Record *r,const char *k,int64_t v){unsigned i;const char *const *ks=record_keys(r->kind);for(i=0;ks[i]&&i<64;i++)if(!strcmp(ks[i],k)){r->values[i]=v;r->present|=(uint64_t)1<<i;break;}}
static void reply(G4Port *p,uint16_t id,uint8_t state){if(!id)return;if(p->reply_count==16){p->overflow_count++;return;}p->replies_id[p->reply_head]=id;p->replies_state[p->reply_head]=state;p->reply_head=(p->reply_head+1)%16;p->reply_count++;}
int g4_response(G4Port *p,uint16_t *id,uint8_t *state){uint32_t irq=lock();if(!p->reply_count){unlock(irq);return 0;}*id=p->replies_id[p->reply_tail];*state=p->replies_state[p->reply_tail];p->reply_tail=(p->reply_tail+1)%16;p->reply_count--;unlock(irq);return 1;}
/* These states mirror G4 f70489a LdoLink_CtrlState_t. DCDC/PERMIT
 * readiness belongs to G4; H7 never forces PERMIT to bypass WAIT_PERMIT. */
int g4_start_wait(const G4Telemetry *t, uint8_t phase) {
  return phase == G4_OUTPUT_STARTING && !t->out && !t->fault_latch && t->ctrl <= 3;
}
uint32_t g4_blocking_g0_faults(const G4Telemetry *t, uint8_t phase) {
  uint32_t mask = t->g0_fault;
  int idle = phase == G4_OUTPUT_IDLE && !t->out && !t->want &&
             !t->fault_latch && (t->ctrl == 0 || t->ctrl == 10 || t->ctrl == 11);
  if (t->kill) mask |= G4_G0_POWER_KILL;
  if (idle || g4_start_wait(t, phase))
    mask &= ~(G4_G0_MEAS_LOST | G4_G0_POWER_KILL | G4_G0_VIN_LOW);
  /* G4 SEND_OUT_ON/WAIT_OUT_ON_ACK can return to WAIT_PERMIT on kill.
     A kill sample is not a running trip before output/ON confirmation.
     Only POWER_KILL gets this wider exception; measurement safety does not. */
  if (phase == G4_OUTPUT_STARTING && !t->out && !t->fault_latch && t->ctrl < 9)
    mask &= ~G4_G0_POWER_KILL;
  if (phase == G4_OUTPUT_STOPPING && !t->out && !t->fault_latch && t->ctrl < 12)
    mask &= ~(G4_G0_MEAS_LOST | G4_G0_POWER_KILL | G4_G0_VIN_LOW);
  return mask;
}
static void decode(G4Port *p,uint8_t type,uint8_t seq,const uint8_t *d,unsigned n,uint32_t now){
 unsigned i,k;G4Record r;uint32_t irq;
 if(type==0x81||type==0x82){if(n!=(type==0x81?1u:2u)){p->parse_error_count++;return;}if(type==0x82){p->nack_valid=1;p->nack_type=d[0];p->nack_seq=seq;p->nack_reason=d[1];p->nack_ms=now;p->nack_matched=p->transaction_active[seq]==1&&p->transaction_type[seq]==d[0];snprintf(p->last_rx,sizeof(p->last_rx),"NACK TYPE=%02X SEQ=%u reason=%u %s",d[0],seq,d[1],p->nack_matched?"matched":"unmatched");log_line(p,0,p->last_rx,now);}if(p->transaction_active[seq]!=1||p->transaction_type[seq]!=d[0])return;irq=lock();reply(p,p->transaction_id[seq],type==0x81?2:3);p->transaction_active[seq]=0;for(i=0;i<2;i++)if(p->inflight_len[i]&&p->inflight_seq[i]==seq)p->inflight_len[i]=0;if(type==0x81)p->ack_count++;else p->err_count++;unlock(irq);return;}
 if(type==0x20){if(n<1||n>96)return;memcpy(p->last_rx,d,n<sizeof(p->last_rx)?n:sizeof(p->last_rx)-1);p->last_rx[n<sizeof(p->last_rx)?n:sizeof(p->last_rx)-1]=0;log_line(p,0,p->last_rx,now);return;}
 k=type-0x10;if(k>3||n!=(k==2?64u:k==3?32u:72u)){p->parse_error_count++;return;}
 if(p->rx_seen[k]&&p->rx_seq[k]==seq)return;
 memset(&r,0,sizeof(r));r.kind=k;r.valid=1;r.ms=now;
#define F(key,v) field(&r,key,(v))
#define U(key,off) F(key,u32(d+off))
#define S(key,off) F(key,(int32_t)u32(d+off))
#define W(key,off) F(key,u16(d+off))
#define I(key,off) F(key,(int16_t)u16(d+off))
#define B(key,off,bit) F(key,(d[off]>>(bit))&1)
 if(k==0){G4Telemetry t;memset(&t,0,sizeof(t));t.valid=1;t.ms=now;t.serial=p->telemetry.serial+1;t.vin_mv=u32(d);t.rail_mv=u32(d+4);t.set_mv=u32(d+16);t.ilim_ma=u32(d+20);t.vout_mv=u32(d+24);t.iout_ma=(int32_t)u32(d+28);t.g0_vin_mv=u32(d+32);t.g0_vset_mv=u32(d+36);t.g0_iset_ma=u32(d+40);t.g0_age_ms=u16(d+52);t.g0=(d[67]>>6)&1;t.g0_tlm=t.g0;t.out=d[66]&1;t.want=(d[66]>>1)&1;t.permit=(d[66]>>5)&1;t.run=(d[66]>>6)&1;t.remote=(d[67]>>3)&1;t.stage_en=d[67]&1;t.ps_en=(d[67]>>1)&1;t.g0_fault=u32(d+58);t.fault=u32(d+62);t.mode=d[69];t.kill=(d[66]>>2)&1;t.ctrl=d[68];t.fault_latch=(d[67]>>7)&1;t.vpre_req_mv=u32(d+44);t.vpre_cmd_mv=u32(d+48);
 U("vin_mv",0);U("vout_mv",4);S("i_buck_ma",8);S("i_boost_ma",12);U("set_mv",16);U("ilim_ma",20);U("g0_vout_mv",24);U("g0_iout_ma",28);U("vpre_req_mv",44);U("vpre_cmd_mv",48);W("g0_age_ms",52);W("duty_a_x10",54);W("duty_c_x10",56);U("g0_fault",58);U("fault",62);
 B("g0_out",66,0);B("g0_want",66,1);B("g0_kill",66,2);B("g0_outoff",66,3);B("permit",66,5);B("run",66,6);B("reg_ok",66,7);B("stage_en",67,0);B("ps_en",67,1);B("flt",67,2);B("rem_sense",67,3);B("ucc_a",67,4);B("ucc_c",67,5);B("g0",67,6);F("g0_ctrl",d[68]);F("mode",d[69]);F("g0_tlm",t.g0);
 if(d[71])t.g0_age_ms=0xffff;
 if(t.g0_fault&256){unsigned a;const char *bad[]={"g0_vout_mv","g0_iout_ma"};for(a=0;a<2;a++){unsigned j;for(j=0;keys_t[j];j++)if(!strcmp(keys_t[j],bad[a]))r.present&=~((uint64_t)1<<j);}}
 irq=lock();p->telemetry=t;
 if(p->output_phase==G4_OUTPUT_STARTING&&(t.out||t.ctrl==9))p->output_phase=G4_OUTPUT_RUNNING;
 /* Accumulate actionable faults until the app consumes them: a healthy
    frame later in the same RX batch must not erase a running fault. */
 p->event_fault|=t.fault;p->event_g0_fault|=g4_blocking_g0_faults(&t,p->output_phase);
 p->event_ctrl_fault|=t.fault_latch||t.ctrl==12;
 if((p->output_phase==G4_OUTPUT_RUNNING||p->output_phase==G4_OUTPUT_STARTING||t.out) && !g4_start_wait(&t,p->output_phase) &&
    (!t.g0||t.g0_age_ms>500U))p->event_g0_stale=1;
 if(!p->event_meter.valid && (p->event_fault||p->event_g0_fault||p->event_ctrl_fault||p->event_g0_stale)){
   p->event_meter=t;p->event_phase=p->output_phase;
 }
 p->records[k]=r;unlock(irq);
 }else if(k==1){G4Battery b;memset(&b,0,sizeof(b));b.valid=1;b.ms=now;b.present=d[0];b.configured=d[1];b.fault=u32(d+4);b.chg=d[22];b.dsg=d[23];b.series=d[25];b.pack_mv=u16(d+44);b.stack_mv=u16(d+46);b.current_ma=(int32_t)u32(d+48);b.sample=d[52];for(i=0;i<5;i++){char key[10];b.cell_mv[i]=(int16_t)u16(d+26+2*i);snprintf(key,sizeof(key),"c%u_mv",i+1);F(key,b.cell_mv[i]);}
 F("bms",d[0]);F("cfg",d[1]);F("st",d[2]);F("alert",d[3]);U("fault",4);W("alarm",8);F("sa",d[10]);F("sb",d[11]);F("sc",d[12]);F("fet",d[13]);W("manuf",14);F("init_step",d[16]);F("cfg_fail",d[17]);W("vcell_rb",18);W("batt",20);F("chg",d[22]);F("dsg",d[23]);F("fets",d[24]);F("series",d[25]);I("min_mv",36);I("max_mv",38);I("dV_mv",40);W("sum_mv",42);W("pack_mv",44);W("stack_mv",46);S("i_pack_ma",48);F("sample",d[52]);if(d[71]&8)S("passq_mah",56);if((d[71]&1)&&u16(d+64)!=0xffff)W("soc_permille",64);I("cc1_ma",66);if(u16(d+68))I("int_temp_dK",68);F("balance_mask",d[70]);F("soc_flags",d[71]);irq=lock();p->battery=b;p->records[k]=r;unlock(irq);
 }else if(k==2){G4Charger c;memset(&c,0,sizeof(c));c.valid=1;c.ms=now;c.online=d[0];c.vbat_mv=u32(d+12);c.vsys_mv=u32(d+16);c.ibat_ma=(int32_t)u32(d+20);c.vbus_mv=u32(d+32);c.fault=d[4];c.otg=(d[1]>>3)&1;c.plug=(d[1]>>7)&1;c.pd_role=d[10];c.pd_mv=u32(d+56);c.pd_ma=u32(d+60);
 F("bq_ok",d[0]);B("bq_in",1,0);B("bq_pre",1,1);B("bq_fast",1,2);B("bq_otg",1,3);B("bq_iindpm",1,4);B("bq_vindpm",1,5);B("rst_busy",1,6);B("plug",1,7);W("bq_st",2);F("bq_fault",d[4]);F("cc1",d[5]);F("cc2",d[6]);F("role",d[7]);F("conn",d[8]);F("typec",d[9]);F("pd_role",d[10]);U("bq_vbat_mv",12);U("bq_vsys_mv",16);S("bq_ibat_ma",20);U("bq_ichg_ma",24);U("bq_idchg_ma",28);U("bq_vbus_mv",32);U("bq_iin_ma",36);U("bq_vreg_mv",40);U("bq_ichg_set_ma",44);U("bq_iin_set_ma",48);U("tps_vbus_mv",52);U("pd_mv",56);U("pd_ma",60);irq=lock();p->charger=c;p->records[k]=r;unlock(irq);
 }else{U("dac_cv_mv",0);U("dac_cc_mv",4);for(i=0;i<4;i++)if((int16_t)u16(d+8+i*2)!=INT16_MIN){char key[4];snprintf(key,sizeof(key),"t%u",i+1);I(key,8+i*2);}F("fan",d[16]);F("pgood",d[17]);F("bleed",d[18]);F("valid",d[19]);if(d[19]){if(u16(d+20)!=0xffff)W("local_mv",20);if(u16(d+22)!=0xffff)W("remote_p_mv",22);if(u16(d+24)!=0xffff)W("remote_n_mv",24);F("sense_code",d[26]);F("sense_flags",d[27]);}if(u16(d+28)!=0xffff)W("fan_rpm",28);irq=lock();p->records[k]=r;unlock(irq);}
 memcpy(p->raw_payload[k],d,n);p->raw_len[k]=(uint8_t)n;
 p->rx_seq[k]=seq;p->rx_seen[k]=1;p->seen_rx=1;p->last_rx_ms=now;p->rx_lines++;
#undef F
#undef U
#undef S
#undef W
#undef I
#undef B
}
void g4_reset_parser(G4Port *p){p->frame_len=0;}
void g4_rx_bytes(G4Port *p,const uint8_t *d,size_t n,uint32_t now){size_t i;if(!p)return;if(p->frame_len&&now-p->parser_ms>10)g4_reset_parser(p);p->parser_ms=now;p->rx_bytes+=(uint32_t)n;for(i=0;i<n;i++){uint8_t b=d[i];if(p->frame_len==0){if(b==0xa5)p->frame[p->frame_len++]=b;continue;}if(p->frame_len==1){if(b==0x5a)p->frame[p->frame_len++]=b;else p->frame_len=b==0xa5?1:0;continue;}if(p->frame_len==2&&(b<2||b>115)){p->frame_len=b==0xa5?1:0;p->parse_error_count++;continue;}p->frame[p->frame_len++]=b;if(p->frame_len==p->frame[2]+5){unsigned len=p->frame[2];if(g4_crc16(p->frame+2,len+1)==u16(p->frame+len+3))decode(p,p->frame[3],p->frame[4],p->frame+5,len-2,now);else p->parse_error_count++;p->frame_len=0;}}}
void g4_init(G4Port *p){if(p){memset(p,0,sizeof(*p));p->sequence=1;}}
void g4_link_up(G4Port *p){p->link=G4_LINK_ONLINE;}
void g4_link_lost(G4Port *p){p->link=G4_LINK_OFFLINE;}
static int command(G4Port *p,uint8_t type,const uint8_t *d,size_t n,uint16_t id){uint32_t irq;uint8_t seq,len,f[120];if(!p)return 0;irq=lock();if(type==2&&(p->inflight_len[1]||p->count)){unsigned j;if(p->inflight_len[1]){unlock(irq);return 0;}for(j=0;j<p->count;j++)if(p->wire_queue[(p->tail+j)%10][3]==2){unlock(irq);return 0;}}for(seq=p->sequence;p->transaction_active[seq];seq++){if((uint8_t)(seq+1)==p->sequence){unlock(irq);return 0;}}p->sequence=seq+1;len=(uint8_t)g4_frame(f,type,seq,d,n);if(type==3||(type==8&&n&&d[0]==0)){unsigned j,kept=0;if(p->safety_count==8){unlock(irq);return 0;}
 /* Never let an ON that was queued before OFF start afterwards. */
 for(j=0;j<p->count;j++){unsigned from=(p->tail+j)%10,to=(p->tail+kept)%10;if(p->wire_queue[from][3]==2){reply(p,p->wire_id[from],3);p->transaction_active[p->wire_queue[from][4]]=0;continue;}if(from!=to){memcpy(p->wire_queue[to],p->wire_queue[from],p->wire_len[from]);p->wire_len[to]=p->wire_len[from];p->wire_id[to]=p->wire_id[from];}kept++;}p->count=kept;p->head=(p->tail+kept)%10;
 if(p->inflight_len[1]){reply(p,p->inflight_id[1],3);p->transaction_active[p->inflight_seq[1]]=0;p->inflight_len[1]=0;}
 memcpy(p->safety_frames[p->safety_head],f,len);p->safety_lengths[p->safety_head]=len;p->safety_ids[p->safety_head]=id;p->safety_head=(p->safety_head+1)%8;p->safety_count++;p->safety_pending=1;}else if(type==1){if(p->has_set){reply(p,p->pending_id,3);p->transaction_active[p->pending_seq]=0;}memcpy(p->pending_frame,f,len);p->pending_len=len;p->pending_seq=seq;p->pending_id=id;p->has_set=1;}else{if(p->count==10){unlock(irq);return 0;}memcpy(p->wire_queue[p->head],f,len);p->wire_len[p->head]=len;p->wire_id[p->head]=id;p->head=(p->head+1)%10;p->count++;}p->transaction_active[seq]=2;p->transaction_id[seq]=id;p->transaction_type[seq]=type;unlock(irq);return 1;}
void g4_process(G4Port *p,uint32_t now){unsigned i,j;uint32_t irq=lock();p->link=!p->records[0].valid?G4_LINK_OFFLINE:now-p->records[0].ms>50?G4_LINK_STALE:G4_LINK_ONLINE;for(i=0;i<256;i++)if(p->transaction_active[i]==1&&now-p->transaction_ms[i]>(p->transaction_type[i]==2?8000u:800u)){reply(p,p->transaction_id[i],4);p->transaction_active[i]=0;for(j=0;j<2;j++)if(p->inflight_len[j]&&p->inflight_seq[j]==i)p->inflight_len[j]=0;}unlock(irq);}
int g4_pop_frame(G4Port *p,uint8_t *o,size_t n,uint32_t now){uint8_t len=0,*src=0,type;uint16_t id=0;uint32_t irq=lock();if(p->safety_count){src=p->safety_frames[p->safety_tail];len=p->safety_lengths[p->safety_tail];id=p->safety_ids[p->safety_tail];p->safety_tail=(p->safety_tail+1)%8;p->safety_count--;p->safety_pending=p->safety_count!=0;}else if(p->has_set&&!p->inflight_len[0]){src=p->pending_frame;len=p->pending_len;id=p->pending_id;p->has_set=0;}else if(p->count){src=p->wire_queue[p->tail];len=p->wire_len[p->tail];id=p->wire_id[p->tail];p->tail=(p->tail+1)%10;p->count--;}if(!len||n<len){unlock(irq);return 0;}memcpy(o,src,len);type=o[3];if(type==7)p->transaction_active[o[4]]=0;if(type!=7){p->transaction_id[o[4]]=id;p->transaction_type[o[4]]=type;p->transaction_ms[o[4]]=now;p->transaction_active[o[4]]=1;}if(type==1||type==2){unsigned i=type==2;memcpy(p->inflight[i],o,len);p->inflight_len[i]=len;p->inflight_seq[i]=o[4];p->inflight_id[i]=id;p->inflight_ms[i]=now;}else{p->active_id=o[4];p->awaiting=1;snprintf(p->active_cmd,sizeof(p->active_cmd),"%02X",type);}p->last_tx_id=id;p->tx_count++;snprintf(p->last_tx,sizeof(p->last_tx),"BIN type=%02X seq=%u",type,o[4]);log_line(p,1,p->last_tx,now);unlock(irq);return len;}
int g4_pop_tx(G4Port *p,char *o,size_t n,uint32_t now){(void)p;(void)o;(void)n;(void)now;return 0;}
int g4_set_limits(G4Port *p,uint32_t mv,uint32_t ma,uint16_t id){uint8_t d[8];if(mv>27000||ma>5000)return 0;put32(d,mv);put32(d+4,ma);return command(p,1,d,8,id);}
int g4_set_mv(G4Port *p,uint32_t mv,uint16_t id){return g4_set_limits(p,mv,p->telemetry.ilim_ma,id);}
int g4_ilim_ma(G4Port *p,uint32_t ma,uint16_t id){return g4_set_limits(p,p->telemetry.set_mv,ma,id);}
int g4_on(G4Port *p,uint16_t id){return command(p,2,0,0,id);}
int g4_off(G4Port *p,uint16_t id){return command(p,3,0,0,id);}
int g4_permit(G4Port *p,int v,uint16_t id){uint8_t d=v!=0;return command(p,8,&d,1,id);}
int g4_remote(G4Port *p,int v,uint16_t id){uint8_t d=v!=0;return command(p,9,&d,1,id);}
int g4_usb_role(G4Port *p,const char *s,uint16_t id){uint8_t d;if(!strcmp(s,"AUTO"))d=0;else if(!strcmp(s,"SINK"))d=1;else if(!strcmp(s,"SOURCE"))d=2;else return 0;return command(p,11,&d,1,id);}
int g4_simple(G4Port *p,const char *s,uint16_t id){uint8_t d;if(!strcmp(s,"CLR")||!strcmp(s,"CLEAR"))return command(p,4,0,0,id);if(!strcmp(s,"STATUS")||!strcmp(s,"PING"))return command(p,5,0,0,id);if(!strcmp(s,"DIAG"))return command(p,7,0,0,id);if(!strcmp(s,"BMS")||!strcmp(s,"BMS FORCE")){d=!strcmp(s,"BMS FORCE");return command(p,10,&d,1,id);}return 0;}
int g4_service_line(G4Port *p,const char *s,uint16_t id){size_t n=s?strlen(s):0;if(!p->service_mode||n<1||n>96)return 0;return command(p,0x21,(const uint8_t*)s,n,id);}
int g4_enqueue(G4Port *p,const char *s,int safety,uint16_t id){(void)safety;return g4_simple(p,s,id);}
int g4_verbose(G4Port *p,int v,uint16_t id){(void)v;return g4_simple(p,"DIAG",id);}
int g4_pps(G4Port *p,uint32_t mv,uint32_t ma,uint16_t id){(void)p;(void)mv;(void)ma;(void)id;return 0;}
void g4_set_pps_gates(G4Port *p,int a,int b,int c){p->pps_ctl=a;p->sink_role=b;p->apdo_selected=c;}
int g4_pps_allowed(const G4Port *p){(void)p;return 0;}
void g4_cancel_pending(G4Port *p){uint32_t irq=lock();p->head=p->tail=p->count=0;p->has_set=p->has_ilim=p->has_pps=0;p->inflight_len[0]=p->inflight_len[1]=0;memset(p->transaction_active,0,sizeof(p->transaction_active));unlock(irq);}
const char *g4_record_key(uint8_t k,unsigned i){const char *const *ks;unsigned j;if(k>3)return 0;ks=record_keys(k);for(j=0;ks[j];j++)if(j==i)return ks[j];return 0;}
void g4_raw_snapshot(const G4Port *p,uint8_t k,char *o,size_t n){uint32_t irq;if(!p||k>3||!n)return;irq=lock();{unsigned i;size_t used=0;o[0]=0;for(i=0;i<p->raw_len[k]&&used+4<n;i++){int count=snprintf(o+used,n-used,"%02X ",p->raw_payload[k][i]);if(count<0)break;used+=(size_t)count;}}unlock(irq);}


int g4_power_shutdown(G4Port *p,uint16_t id){static const uint8_t text[]="BMS SHUTDOWN";return command(p,0x21,text,sizeof(text)-1,id);}
