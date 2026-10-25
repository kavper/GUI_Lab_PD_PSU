/* Native web features share the LCD model. Network queues; GUI executes.
 * Single producer / single consumer ring; no blocking or peripheral I/O here. */
#include "psu_web_features.h"
#include "psu_app.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
enum {OP,INDEX,V,I,T,S,E,A,MODE,LOOPS,STOP,CHEM,CELLS,TERM,TARGET,POLARITY,DARK,ACCENT,VOLUME,SOUND,TONE,FEEDBACK,VALUE,FIELDS};
static const char *keys[FIELDS]={"op","index","v","i","t","s","e","a","mode","loops","stop","chem","cells","term","target","polarity","dark","accent","volume","sound","tone","feedback","value"};
typedef struct {uint32_t value[FIELDS],mask,id;} Job;
static Job queue[4];
static volatile unsigned head,tail,gui_events;
static uint32_t next_id,last_id;
static int last_ok;
static char last_reason[96]="No remote operation";
void psu_web_features_init(void){head=tail=gui_events=0;next_id=last_id=0;last_ok=0;strcpy(last_reason,"No remote operation");}
#define BIT(k) (1UL<<(k))
#define BARRIER() __asm__ volatile("" ::: "memory")
static void space(const char **p){while(isspace((unsigned char)**p))++*p;}
static int parse(const char *body,Job *j) {
    if(!body)return 0;
    const char *p=body;memset(j,0,sizeof(*j));space(&p);if(*p++!='{')return 0;
    for(unsigned n=0;n<FIELDS;n++) {
        space(&p);if(*p++!='"')return 0;const char *key=p;
        while(*p&&*p!='"')p++;
        if(!*p)return 0;
        unsigned k;for(k=0;k<FIELDS;k++)if(strlen(keys[k])==(size_t)(p-key)&&!memcmp(key,keys[k],p-key))break;
        if(k==FIELDS||(j->mask&BIT(k)))return 0;
        p++;space(&p);if(*p++!=':')return 0;space(&p);
        if(!isdigit((unsigned char)*p)||(*p=='0'&&isdigit((unsigned char)p[1])))return 0;
        uint32_t value=0;
        do{unsigned d=*p++-'0';if(value>(UINT32_MAX-d)/10U)return 0;value=value*10+d;}while(isdigit((unsigned char)*p));
        j->value[k]=value;j->mask|=BIT(k);space(&p);
        if(*p=='}'){p++;space(&p);return !*p&&(j->mask&BIT(OP));}
        if(*p++!=',')return 0;
    }
    return 0;
}
static uint32_t fields_for(unsigned op) {
    switch(op) {
    case 1:return BIT(INDEX)|BIT(V)|BIT(I); /* store preset */
    case 2:case 3:case 12:return BIT(INDEX); /* apply/store current/remove row */
    case 4:case 11:case 14:case 16:case 22:case 41:case 50:case 52:case 53:case 54:case 57:return 0;
    case 10:return BIT(INDEX)|BIT(V)|BIT(I)|BIT(T)|BIT(S)|BIT(E)|BIT(A);
    case 13:return BIT(MODE)|BIT(LOOPS)|BIT(STOP);
    case 15:case 30:case 51:case 56:return BIT(VALUE);
    case 20:return BIT(CHEM)|BIT(CELLS)|BIT(I)|BIT(TERM)|BIT(TARGET);
    case 21:return BIT(POLARITY);
    case 40:return BIT(DARK)|BIT(ACCENT)|BIT(VOLUME)|BIT(SOUND)|BIT(TONE)|BIT(FEEDBACK);
    default:return UINT32_MAX;
    }
}
int psu_web_features_submit(const char *body,char *out,size_t size) {
    Job j;unsigned op;
    if(!parse(body,&j)||(op=j.value[OP],fields_for(op)==UINT32_MAX)||j.mask!=(fields_for(op)|BIT(OP)))
        return snprintf(out,size,"{\"result\":\"rejected\",\"reason\":\"invalid fields\"}");
    unsigned next=(head+1)%4;
    if(next==tail)return snprintf(out,size,"{\"result\":\"rejected\",\"reason\":\"queue busy\"}");
    j.id=++next_id;if(!j.id)j.id=++next_id;
    queue[head]=j;BARRIER();head=next;
    return snprintf(out,size,"{\"result\":\"queued\",\"job\":%lu}",(unsigned long)j.id);
}
unsigned psu_web_features_gui_events(void){unsigned e=gui_events;gui_events=0;return e;}
void psu_web_features_poll(void) {
    if(tail==head)return;
    BARRIER();Job j=queue[tail];tail=(tail+1)%4;
    uint32_t *v=j.value;unsigned op=v[OP];int ok=0;const char *reason="Rejected: invalid values or active operation";
    PsuSnapshot snap;psu_snapshot(&snap);PsuSequencer *seq=psu_sequencer();PsuCharger *chg=psu_charger();
    if(!psu_remote_enabled()){reason="Remote control disabled locally";goto done;}
    switch(op) {
    case 1:ok=v[INDEX]<PSU_PRESET_COUNT&&psu_preset_set(v[INDEX],v[V],v[I]);break;
    case 2:{const PsuPreset *p=v[INDEX]<PSU_PRESET_COUNT?psu_preset_get(v[INDEX]):0;
        if(p&&!chg->running&&!psu_seq_edit_locked(seq))ok=psu_app_set_limits(p->voltage_mv,p->current_ma,PSU_SRC_WEB);
        break;}
    case 3:ok=v[INDEX]<PSU_PRESET_COUNT&&psu_preset_save_current(v[INDEX],0);break;
    case 4:psu_preset_reset_defaults();ok=1;break;
    case 10:if(v[INDEX]<seq->count&&v[E]<=1&&v[A]<=PSU_STEP_OFF){
        PsuSeqStep step={v[V],v[I],v[T],v[S],(uint8_t)v[E],(uint8_t)v[A]};ok=psu_seq_set_step(seq,v[INDEX],&step);}break;
    case 11:ok=psu_seq_add(seq);break;
    case 12:if(v[INDEX]<seq->count&&!psu_seq_edit_locked(seq)){seq->selected=v[INDEX];ok=psu_seq_remove_selected(seq);}break;
    case 13:if(!psu_seq_edit_locked(seq)&&v[MODE]<=2&&v[LOOPS]>=1&&v[LOOPS]<=65535&&v[STOP]<=2){
        seq->mode=v[MODE];seq->loops_requested=v[LOOPS];seq->stop_action=v[STOP];ok=1;}break;
    case 14:if(!chg->running&&!psu_seq_edit_locked(seq)&&!snap.fault_latched&&snap.g4_link==G4_LINK_ONLINE){
        for(unsigned n=0;n<seq->count;n++)if(seq->steps[n].enabled){if(seq->steps[n].output_action==PSU_STEP_KEEP)seq->steps[n].output_action=PSU_STEP_ON;break;}
        ok=psu_seq_start(seq,psu_app_now());}break;
    case 15:if(v[VALUE]==0&&seq->run==PSU_SEQ_RUN){psu_seq_pause(seq,psu_app_now());ok=1;}
        if(v[VALUE]==1&&seq->run==PSU_SEQ_PAUSE&&!chg->running){psu_seq_resume(seq,psu_app_now());ok=1;}break;
    case 16:psu_seq_stop(seq,psu_app_now(),0);ok=1;break;
    case 20:if(!chg->running&&!psu_seq_edit_locked(seq)&&v[CHEM]<=CHEM_LEAD&&v[CELLS]>=1&&v[CELLS]<=8&&v[I]>=1&&v[I]<=5000&&v[TERM]>=1&&v[TERM]<=v[I]){
        PsuChgProfile profile;psu_chg_profile_defaults(&profile,v[CHEM]);profile.cells=v[CELLS];
        if(profile.cells<=psu_chg_max_cells(&profile)&&psu_chg_set_target(&profile,v[TARGET])) {
            profile.cc_ma=v[I];profile.term_ma=v[TERM];if(profile.precharge_ma>v[I])profile.precharge_ma=v[I];
            profile.confirmed=profile.polarity_checked=0;chg->profile=profile;ok=1;
        }}break;
    case 21:if(v[POLARITY]==1&&!snap.output_requested&&!snap.output_confirmed&&!psu_seq_edit_locked(seq)&&!chg->running){
        chg->profile.confirmed=chg->profile.polarity_checked=1;ok=psu_app_start_charging();
        if(!ok){chg->profile.confirmed=chg->profile.polarity_checked=0;reason=chg->reason;}
        }else reason="Output must be OFF; confirm polarity and stop other automation";break;
    case 22:ok=psu_app_shutdown();break;
    case 30:if(v[VALUE]<=2){const char *roles[]={"AUTO","SINK","SOURCE"};ok=psu_app_usb_role(roles[v[VALUE]],PSU_SRC_WEB);}break;
    case 40:if(v[DARK]<=1&&v[ACCENT]<=3&&v[VOLUME]<=100&&v[SOUND]<=1&&v[TONE]<=2&&v[FEEDBACK]<=3){
        PsuAppearance a={(uint8_t)v[DARK],(uint8_t)v[ACCENT],(uint8_t)v[VOLUME],(uint8_t)v[SOUND],(uint8_t)v[TONE],(uint8_t)v[FEEDBACK]};psu_appearance_set(&a);gui_events|=1;ok=1;}break;
    case 41:gui_events|=2;ok=1;break;
    case 50:ok=psu_app_clear_fault();break;
    case 51:if(v[VALUE]<=1)ok=psu_app_service(v[VALUE]);break;
    case 52:ok=psu_app_service_cmd("HELP");break;
    case 53:ok=g4_simple(psu_g4(),"STATUS",0);break;
    case 54:ok=psu_app_power_shutdown_from(PSU_SRC_WEB);break;
    case 56:{
        G4Record r;int64_t flags=0;g4_record_snapshot(psu_g4(),G4_RECORD_AUX,&r);
        if(v[VALUE]<=1&&r.valid&&psu_app_now()-r.ms<=1000&&g4_record_value(&r,"sense_flags",&flags)&&!(flags&4))
            ok=g4_remote(psu_g4(),v[VALUE],0);
        break;}
    case 57:ok=psu_app_bms_cmd("BMS");break;
    }
done:
    last_id=j.id;last_ok=ok;snprintf(last_reason,sizeof(last_reason),"%s",ok?"Applied to instrument model; controller acknowledgement is separate":reason);
}

typedef struct {char *out;size_t size,used;int failed;} Json;
static void add(Json *w,const char *format,...) {
    if(w->failed)return;
    va_list ap;va_start(ap,format);
    int n=vsnprintf(w->out+w->used,w->size-w->used,format,ap);va_end(ap);
    if(n<0||(size_t)n>=w->size-w->used){w->failed=1;return;}w->used+=n;
}
static void string(Json *w,const char *s) {
    add(w,"\"");for(;*s;s++){
        unsigned char c=*s;
        if(c=='"'||c=='\\')add(w,"\\%c",c);
        else if(c<32)add(w,"\\u%04x",c);else add(w,"%c",c);
    }add(w,"\"");
}
static void integer64(Json *w,int64_t value) {
    char digits[24];unsigned n=0;uint64_t magnitude=value<0?0-(uint64_t)value:(uint64_t)value;
    do{digits[n++]=(char)('0'+magnitude%10);magnitude/=10;}while(magnitude);
    if(value<0)add(w,"-");
    while(n)add(w,"%c",digits[--n]);
}
static void record(Json *w,unsigned kind) {
    G4Record r;g4_record_snapshot(psu_g4(),kind,&r);
    uint32_t age=psu_app_now()-r.ms;add(w,"{\"valid\":%u,\"age\":%lu,\"ms\":%lu,\"values\":{",r.valid&&age<=(kind==0?G4_METER_FRESH_MS:1000),(unsigned long)age,(unsigned long)r.ms);
    unsigned emitted=0;
    for(unsigned k=0;k<64;k++)if(r.present&(1ULL<<k)) {
        const char *key=g4_record_key(kind,k);if(!key||!key[0])continue;
        if(emitted++)add(w,",");
        string(w,key);add(w,":");integer64(w,r.values[k]);
    }add(w,"},\"raw\":");char raw[1600];g4_raw_snapshot(psu_g4(),kind,raw,sizeof(raw));string(w,raw);add(w,"}");
}
int psu_web_features_state(const char *path,char *out,size_t size) {
    Json w={out,size,0,0};if(!out||size<32)return 0;
    if(!strcmp(path,"/api/diagnostics")) {
        add(&w,"{\"frames\":[");for(unsigned k=0;k<4;k++){if(k)add(&w,",");record(&w,k);}add(&w,"]}");
    }else if(!strcmp(path,"/api/pd")) {
        G4PdCapabilities c;g4_pd_capabilities_snapshot(psu_g4(),&c);
        add(&w,"{\"valid\":%u,\"age\":%lu,\"flags\":%u,\"generation\":%lu,\"role\":%u,\"rdo\":%lu,\"active\":%lu,\"pdo\":[",c.valid,(unsigned long)(psu_app_now()-c.ms),c.flags,(unsigned long)c.attach_generation,c.role,(unsigned long)c.rdo,(unsigned long)c.active_pdo);
        for(unsigned i=0;i<c.count&&i<7;i++)add(&w,"%s%lu",i?",":"",(unsigned long)c.pdo[i]);
        add(&w,"]}");
    }else if(!strcmp(path,"/api/features")) {
        PsuAppearance a;psu_appearance_get(&a);PsuSequencer *s=psu_sequencer();PsuCharger *c=psu_charger();
        add(&w,"{\"job\":%lu,\"ok\":%u,\"reason\":",(unsigned long)last_id,last_ok);string(&w,last_reason);
        add(&w,",\"presets\":[");for(unsigned i=0;i<PSU_PRESET_COUNT;i++){const PsuPreset *p=psu_preset_get(i);if(i)add(&w,",");add(&w,"{\"v\":%lu,\"i\":%lu,\"name\":",(unsigned long)p->voltage_mv,(unsigned long)p->current_ma);string(&w,p->name);add(&w,"}");}
        add(&w,"],\"appearance\":{\"dark\":%u,\"accent\":%u,\"volume\":%u,\"sound\":%u,\"tone\":%u,\"feedback\":%u}",a.dark,a.accent,a.sound_volume,a.sound_enabled,a.sound_tone,a.sound_feedback);
        add(&w,",\"seq\":{\"run\":%u,\"mode\":%u,\"loops\":%u,\"stop\":%u,\"step\":%u,\"cycle\":%u,\"locked\":%u,\"status\":",s->run,s->mode,s->loops_requested,s->stop_action,s->step_index,s->loop_index,psu_seq_edit_locked(s));string(&w,s->status);
        add(&w,",\"steps\":[");for(unsigned i=0;i<s->count&&i<PSU_SEQ_MAX_STEPS;i++){const PsuSeqStep *p=&s->steps[i];add(&w,"%s{\"v\":%lu,\"i\":%lu,\"t\":%lu,\"s\":%lu,\"e\":%u,\"a\":%u}",i?",":"",(unsigned long)p->voltage_mv,(unsigned long)p->current_ma,(unsigned long)p->time_ms,(unsigned long)p->slew_mv_per_s,p->enabled,p->output_action);}add(&w,"]}");
        PsuChgProfile *p=&c->profile;
        add(&w,",\"charger\":{\"chem\":%u,\"cells\":%u,\"i\":%lu,\"term\":%lu,\"target\":%lu,\"running\":%u,\"state\":%u,\"elapsed\":%lu,\"mah\":%lu,\"mwh\":%lu,\"status\":",p->chemistry,p->cells,(unsigned long)p->cc_ma,(unsigned long)p->term_ma,(unsigned long)psu_chg_target_mv(p),c->running,c->state,(unsigned long)c->elapsed_ms,(unsigned long)c->delivered_mah,(unsigned long)c->delivered_mwh);
        string(&w,psu_chg_state_name(c->state));add(&w,",\"reason\":");string(&w,c->reason);add(&w,",\"trace\":[");
        for(unsigned i=0;i<c->trace_count&&i<128;i++)add(&w,"%s[%lu,%lu,%lu]",i?",":"",(unsigned long)c->trace_time[i],(unsigned long)c->trace_mv[i],(unsigned long)c->trace_ma[i]);
        add(&w,"]}}");
    }else return 0;
    if(w.failed){snprintf(out,size,"{\"error\":\"response capacity exceeded\"}");return 0;}
    return (int)w.used;
}
