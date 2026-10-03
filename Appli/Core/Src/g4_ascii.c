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
static int prefix(const char *s,const char *v) {size_t n=strlen(v);return !strncmp(s,v,n)&&(s[n]==' '||s[n]==0);}
static const char *value(const char *s,const char *key) {
  size_t n=strlen(key);
  while(*s){while(*s==' ')++s;if(!strncmp(s,key,n)&&s[n]=='=')return s+n+1;while(*s&&*s!=' ')++s;}
  return NULL;
}
static int number(const char *s,const char *key,int64_t *out) {
  const char *v=value(s,key);char *end;long long n;
  if(!v||!*v)return 0;
  errno=0;n=strtoll(v,&end,(!strncmp(v,"0x",2)||!strncmp(v,"0X",2))?16:10);
  if(errno||end==v||(*end&&*end!=' ')||n<INT32_MIN||n>UINT32_MAX)return 0;
  *out=n;return 1;
}
static uint32_t u(const char *s,const char *k) {int64_t n=0;return number(s,k,&n)&&n>=0?(uint32_t)n:0;}
static int32_t si(const char *s,const char *k) {int64_t n=0;return number(s,k,&n)&&n<=INT32_MAX?(int32_t)n:0;}
static int required(const char *s,const char *k) {int64_t n;return number(s,k,&n);}
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
  "min_mv","max_mv","dV_mv","sum_mv","pack_mv","stack_mv","i_pack_ma","i_cc2_ma","sample","alerts","i2c_err", NULL
};
static const char *const keys_tc[] = {
  "bq_ok","bq_vbat_mv","bq_vsys_mv","bq_ibat_ma","bq_ichg_ma","bq_idchg_ma","bq_vbus_mv","bq_iin_ma",
  "bq_vreg_mv","bq_ichg_set_ma","bq_iin_set_ma","bq_st","bq_fault","bq_in","bq_pre","bq_fast","bq_otg",
  "bq_iindpm","bq_vindpm","tps_vbus_mv","cc1","cc2","role","conn","plug","typec","rst","rst_busy",
  "pd_role","pd_mv","pd_ma", NULL
};
static const char *const *record_keys(uint8_t kind) {
  return kind==G4_RECORD_T ? keys_t : kind==G4_RECORD_TB ? keys_tb : keys_tc;
}
static void record_line(G4Port *p, uint8_t kind, const char *line, uint32_t now) {
  G4Record record;
  const char *const *keys=record_keys(kind);
  unsigned i;
  uint32_t irq;
  memset(&record,0,sizeof(record));
  record.kind=kind;record.ms=now;record.valid=1;
  for(i=0;keys[i] && i<64;i++) {
    if(number(line,keys[i],&record.values[i]))record.present|=(uint64_t)1<<i;
    else if(kind==G4_RECORD_T && !strcmp(keys[i],"mode")) {
      const char *m=value(line,"mode");
      if(m){record.values[i]=prefix(m,"CC")?2:prefix(m,"CV")?1:0;record.present|=(uint64_t)1<<i;}
    }
  }
  irq=lock();p->records[kind]=record;snprintf(p->raw_records[kind],G4_RX_LINE_MAX,"%s",line);unlock(irq);
}
int g4_record_value(const G4Record *record,const char *key,int64_t *out) {
  unsigned i;const char *const *keys;
  if(!record||!record->valid||record->kind>G4_RECORD_TC||!key||!out)return 0;
  keys=record_keys(record->kind);
  for(i=0;keys[i] && i<64;i++)if(!strcmp(keys[i],key)) {
    if(!(record->present&((uint64_t)1<<i)))return 0;
    *out=record->values[i];return 1;
  }
  return 0;
}
void g4_record_snapshot(const G4Port *p,uint8_t kind,G4Record *out) {
  uint32_t irq;if(!p||!out||kind>G4_RECORD_TC)return;
  irq=lock();*out=p->records[kind];unlock(irq);
}
static int parse_t(G4Port *p,const char *s,uint32_t now) {
  const char *keys[]={"vin_mv","g0_vout_mv","g0_iout_ma","set_mv","ilim_ma","g0_out","g0_want","g0","g0_age_ms","g0_tlm","fault","g0_fault","fmt"};
  const char *mode=value(s,"mode");size_t i;G4Telemetry t;
  for(i=0;i<sizeof(keys)/sizeof(keys[0]);++i){int64_t n;if(!number(s,keys[i],&n))return 0;if(strcmp(keys[i],"g0_iout_ma")&&n<0)return 0;}
  if(u(s,"fmt")!=0||!mode||u(s,"set_mv")>27000||u(s,"ilim_ma")>5000)return 0;
  if(!(prefix(mode,"IDLE")||prefix(mode,"CV")||prefix(mode,"CC")))return 0;
  if(u(s,"g0_vout_mv")>100000||si(s,"g0_iout_ma")>100000||si(s,"g0_iout_ma")< -100000)return 0;
  memset(&t,0,sizeof(t));t.valid=1;t.ms=now;t.serial=p->telemetry.serial+1;
  t.vin_mv=u(s,"vin_mv");t.rail_mv=u(s,"vout_mv");t.set_mv=u(s,"set_mv");t.ilim_ma=u(s,"ilim_ma");
  t.vout_mv=u(s,"g0_vout_mv");t.iout_ma=si(s,"g0_iout_ma");t.g0=u(s,"g0")==1;t.out=u(s,"g0_out")==1;t.want=u(s,"g0_want")==1;
  t.mode=prefix(mode,"CC")?2:prefix(mode,"CV")?1:0;t.fault=u(s,"fault");t.g0_fault=u(s,"g0_fault");
  t.g0_age_ms=u(s,"g0_age_ms");t.g0_rx=u(s,"g0_rx");t.g0_tlm=u(s,"g0_tlm");t.g0_err=u(s,"g0_err");t.g0_uart=u(s,"g0_uart");
  t.permit=u(s,"permit")==1;t.remote=u(s,"rem_sense")==1;t.vpre_req_mv=u(s,"vpre_req_mv");t.vpre_cmd_mv=u(s,"vpre_cmd_mv");
  t.pd=u(s,"pd")==1;t.pd_mv=u(s,"pd_mv");t.pd_ma=u(s,"pd_ma");p->telemetry=t;
  p->telemetry.run=u(s,"run")==1;p->telemetry.stage_en=u(s,"stage_en")==1;p->telemetry.ps_en=u(s,"ps_en")==1;
  record_line(p,G4_RECORD_T,s,now);
  p->event_fault=t.fault|t.g0_fault;
  return 1;
}
static int parse_tb(G4Port *p,const char *s,uint32_t now) {
  G4Battery b;unsigned i;char key[12];
  if(!required(s,"bms")||!required(s,"cfg")||!required(s,"sample")||!required(s,"fault"))return 0;
  memset(&b,0,sizeof(b));b.valid=1;b.ms=now;b.present=u(s,"bms")==1;b.configured=u(s,"cfg")==1;b.sample=u(s,"sample")==1;
  b.fault=u(s,"fault");b.chg=u(s,"chg")==1;b.dsg=u(s,"dsg")==1;b.series=(uint8_t)u(s,"series");
  b.pack_mv=u(s,"pack_mv");b.stack_mv=u(s,"stack_mv");b.current_ma=si(s,"i_pack_ma");
  for(i=0;i<5;i++){snprintf(key,sizeof(key),"c%u_mv",i+1);b.cell_mv[i]=required(s,key)?si(s,key):-1;if(b.cell_mv[i]< -1||b.cell_mv[i]>6000)return 0;}
  p->battery=b;record_line(p,G4_RECORD_TB,s,now);return 1;
}
static int parse_tc(G4Port *p,const char *s,uint32_t now) {
  G4Charger c;if(!required(s,"bq_ok"))return 0;memset(&c,0,sizeof(c));c.valid=1;c.ms=now;
  c.online=u(s,"bq_ok")==1;c.vbat_mv=u(s,"bq_vbat_mv");c.vsys_mv=u(s,"bq_vsys_mv");c.vbus_mv=u(s,"bq_vbus_mv");c.ibat_ma=si(s,"bq_ibat_ma");
  c.fault=u(s,"bq_fault");c.otg=u(s,"bq_otg")==1;c.plug=u(s,"plug")==1;c.pd_role=(uint8_t)u(s,"pd_role");c.pd_mv=u(s,"pd_mv");c.pd_ma=u(s,"pd_ma");p->charger=c;record_line(p,G4_RECORD_TC,s,now);return 1;
}
void g4_init(G4Port *p){if(p){memset(p,0,sizeof(*p));p->next_id=1;}}
void g4_link_up(G4Port *p){if(p){p->link=G4_LINK_ONLINE;p->need_session_init=1;p->init_phase=0;}}
void g4_link_lost(G4Port *p){if(p){p->link=G4_LINK_OFFLINE;p->session_ready=0;}}
static int allowed(const char *s) {
  const char *simple[]={"ON","OFF","CLR","CLEAR","BMS","?","STATUS","ST","HELP","TEL","TEL 0","TEL 100","TEL 200","TEL 250","TEL 500","TEL 1000","TEL 2000","PERMIT 0","PERMIT 1","REMOTE 0","REMOTE 1","REMOTE ON","REMOTE OFF","USB AUTO","USB SINK","USB SOURCE","VERBOSE 0"};
  unsigned i;for(i=0;i<sizeof(simple)/sizeof(simple[0]);i++)if(!strcmp(s,simple[i]))return 1;return 0;
}
int g4_enqueue(G4Port *p,const char *s,int safety,uint16_t id) {
  G4Slot v;uint32_t irq;if(!p||!s||!allowed(s)||strlen(s)>=sizeof(v.text))return 0;
  memset(&v,0,sizeof(v));strcpy(v.text,s);v.cmd_id=id;v.safety=(uint8_t)(safety||!strcmp(s,"OFF")||!strcmp(s,"PERMIT 0"));irq=lock();
  if(v.safety){p->safety=v;p->safety_pending=1;/* OFF must supersede queued ON. */
    if(!strcmp(s,"OFF")||!strcmp(s,"PERMIT 0")){unsigned i;for(i=0;i<p->count;i++){unsigned k=(p->tail+i)%G4_QUEUE_LEN;if(!strcmp(p->queue[k].text,"ON"))p->queue[k].text[0]=0;}}
  }else {if(p->count==G4_QUEUE_LEN){unlock(irq);return 0;}p->queue[p->head]=v;p->head=(p->head+1)%G4_QUEUE_LEN;++p->count;}
  unlock(irq);return 1;
}
int g4_set_limits(G4Port *p,uint32_t mv,uint32_t ma,uint16_t id) {
  char line[40];uint32_t irq;if(!p||mv>27000||ma>5000)return 0;
  snprintf(line,sizeof(line),"SET V=%u.%03u I=%u.%03u",(unsigned)(mv/1000),(unsigned)(mv%1000),(unsigned)(ma/1000),(unsigned)(ma%1000));
  irq=lock();strcpy(p->pending_set,line);p->set_id=id;p->has_set=1;p->retry_used=0;p->set_due_ms=0;unlock(irq);return 1;
}
/* Compatibility callers still produce the single contracted SET command. */
int g4_set_mv(G4Port *p,uint32_t v,uint16_t id){return g4_set_limits(p,v,p?p->telemetry.ilim_ma:0,id);}
int g4_ilim_ma(G4Port *p,uint32_t i,uint16_t id){return g4_set_limits(p,p?p->telemetry.set_mv:0,i,id);}
int g4_off(G4Port *p,uint16_t id){return g4_enqueue(p,"OFF",1,id);}
void g4_cancel_pending(G4Port *p) {
  uint32_t irq;if(!p)return;irq=lock();
  p->head=p->tail=p->count=0;p->has_set=p->has_ilim=p->has_pps=0;
  p->retry_used=1;unlock(irq);
}
int g4_on(G4Port *p,uint16_t id){return g4_enqueue(p,"ON",0,id);}
int g4_usb_role(G4Port *p,const char *r,uint16_t id){char s[24];if(!r)return 0;snprintf(s,sizeof(s),"USB %s",r);return g4_enqueue(p,s,0,id);}
int g4_permit(G4Port *p,int on,uint16_t id){return g4_enqueue(p,on?"PERMIT 1":"PERMIT 0",!on,id);}
int g4_remote(G4Port *p,int on,uint16_t id){return g4_enqueue(p,on?"REMOTE 1":"REMOTE 0",0,id);}
int g4_verbose(G4Port *p,int on,uint16_t id){return on?0:g4_enqueue(p,"VERBOSE 0",0,id);}
int g4_simple(G4Port *p,const char *s,uint16_t id){return g4_enqueue(p,s,0,id);}
int g4_service_line(G4Port *p,const char *s,uint16_t id){return p&&p->service_mode?g4_enqueue(p,s,0,id):0;}
int g4_pps(G4Port *p,uint32_t v,uint32_t i,uint16_t id){(void)p;(void)v;(void)i;(void)id;return 0;}
void g4_set_pps_gates(G4Port *p,int a,int b,int c){(void)p;(void)a;(void)b;(void)c;}
int g4_pps_allowed(const G4Port *p){(void)p;return 0;/* Not in the host contract. */}

void g4_process(G4Port *p,uint32_t now) {
  if(!p)return;
  if(p->seen_rx){uint32_t age=now-p->last_rx_ms;p->link=age>3000?G4_LINK_OFFLINE:age>1500?G4_LINK_STALE:G4_LINK_ONLINE;}
  if(p->awaiting&&now-p->active_ms>G4_CMD_TIMEOUT_MS){p->response_id=p->active_id;p->response_state=4;p->awaiting=0;}
}
int g4_pop_tx(G4Port *p,char *out,size_t n,uint32_t now) {
  G4Slot s;uint32_t irq;if(!p||!out||n<G4_LINE_MAX+2)return 0;memset(&s,0,sizeof(s));irq=lock();
  if(p->safety_pending){s=p->safety;p->safety_pending=0;p->awaiting=0;}
  else if(p->awaiting){unlock(irq);return 0;}
  else if(p->need_session_init){strcpy(s.text,p->init_phase?"STATUS":"TEL 500");if(p->init_phase){p->need_session_init=0;p->session_ready=1;}else p->init_phase=1;}
  else if(p->has_set){
    if((p->tx_count&&now-p->last_pair_ms<G4_SETPOINT_GAP_MS)||(p->set_due_ms&&(int32_t)(now-p->set_due_ms)<0)){unlock(irq);return 0;}
    strcpy(s.text,p->pending_set);s.cmd_id=p->set_id;p->has_set=0;p->last_pair_ms=now;
  }else while(p->count){s=p->queue[p->tail];p->tail=(p->tail+1)%G4_QUEUE_LEN;--p->count;if(s.text[0])break;}
  if(!s.text[0]){unlock(irq);return 0;}
  snprintf(out,n,"%s\r\n",s.text);strcpy(p->last_tx,s.text);strcpy(p->active_cmd,s.text);
  p->last_tx_id=p->active_id=s.cmd_id;p->active_ms=now;p->awaiting=1;++p->tx_count;
  log_line(p,1,s.text,now);unlock(irq);return 1;
}
static int ack_matches(G4Port *p,const char *s) {
  char key[16];size_t n=strcspn(p->active_cmd," ");if(n>=sizeof(key))return 0;
  memcpy(key,p->active_cmd,n);key[n]=0;
  if(!strcmp(key,"REMOTE"))return prefix(s,"OK REMOTE")||prefix(s,"OK LOCAL");
  if(!strcmp(key,"CLEAR"))strcpy(key,"CLR");
  return prefix(s,"OK")&&prefix(s+3,key);
}
static void accept_line(G4Port *p,uint32_t now) {
  char *s=p->rx_acc;int valid=1,recognized=1;
  if(!p->rx_len)return;
  s[p->rx_len]=0;
  if(prefix(s,"TB"))valid=parse_tb(p,s,now);
  else if(prefix(s,"TC"))valid=parse_tc(p,s,now);
  else if(prefix(s,"T")){
    valid=parse_t(p,s,now);
    if(valid&&p->awaiting&&(!strcmp(p->active_cmd,"STATUS")||!strcmp(p->active_cmd,"ST")||!strcmp(p->active_cmd,"?"))){p->awaiting=0;p->response_id=p->active_id;p->response_state=2;}
  }else if(prefix(s,"OK")){
    ++p->ack_count;
    if(p->awaiting&&ack_matches(p,s)){p->awaiting=0;p->response_id=p->active_id;p->response_state=2;}
  }else if(prefix(s,"ERR")){
    ++p->err_count;
    if(p->awaiting){
      if(prefix(p->active_cmd,"SET")&&!p->retry_used&&!p->has_set&&(prefix(s,"ERR CMD")||prefix(s,"ERR LINE"))){
        strcpy(p->pending_set,p->active_cmd);p->set_id=p->active_id;p->has_set=1;p->retry_used=1;p->set_due_ms=now+G4_SETPOINT_GAP_MS;
      }else {p->response_id=p->active_id;p->response_state=3;}
      p->awaiting=0;
    }
  }else if(prefix(s,"E")){if(prefix(s,"E OCP"))p->event_fault|=4;}
  else if(prefix(s,"HELP")){if(p->awaiting&&!strcmp(p->active_cmd,"HELP")){p->awaiting=0;p->response_id=p->active_id;p->response_state=2;}}
  else if(prefix(s,"WARN")||prefix(s,"NACK")||!strncmp(s,"=== Lab_PD_PSU G4 host ready",26)||prefix(s,"boot")||prefix(s,"USART1")){}
  else recognized=0;
  if(recognized){++p->rx_lines;snprintf(p->last_rx,sizeof(p->last_rx),"%s",s);log_line(p,0,s,now);if(valid){p->seen_rx=1;p->last_rx_ms=now;p->link=G4_LINK_ONLINE;}else ++p->parse_error_count;}
  p->rx_len=0;
}
void g4_rx_bytes(G4Port *p,const uint8_t *data,size_t n,uint32_t now) {
  size_t i;if(!p||!data)return;p->rx_bytes+=(uint32_t)n;
  for(i=0;i<n;i++){
    uint8_t c=data[i];
    if(c=='\r'||c=='\n'){if(!p->rx_dropping)accept_line(p,now);p->rx_dropping=0;p->rx_len=0;continue;}
    if(p->rx_dropping)continue;
    if(p->rx_len+1>=G4_RX_LINE_MAX){++p->overflow_count;p->rx_len=0;p->rx_dropping=1;continue;}
    p->rx_acc[p->rx_len++]=(char)c;
  }
}

const char *g4_record_key(uint8_t kind,unsigned index) {
  unsigned i;const char *const *keys;
  if(kind>G4_RECORD_TC)return 0;
  keys=record_keys(kind);
  for(i=0;keys[i];i++){if(i==index)return keys[i];}
  return 0;
}
void g4_raw_snapshot(const G4Port *p,uint8_t kind,char *out,size_t n) {
  uint32_t irq;if(!p||kind>G4_RECORD_TC||!out||!n)return;
  irq=lock();snprintf(out,n,"%s",p->raw_records[kind]);unlock(irq);
}
