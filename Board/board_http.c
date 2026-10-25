/* Small raw-TCP HTTP server. Fixed sessions, bounded parsing, COPY writes.
 * All callbacks execute on the single lwIP owner task; no socket worker heap. */
#include "board_http.h"
#include "board_net.h"
#include "psu_app.h"
#include "psu_web_features.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "remote_page.h"
#define SESSION_COUNT 3
#define REQUEST_MAX 1536

typedef struct {
    struct tcp_pcb *pcb;
    uint32_t activity,unacked;
    unsigned used,parsed,header_offset,header_length,responding;
    size_t body_offset,body_length;
    const char *body;
    char request[REQUEST_MAX],header[256],response[16384];
} Session;
static Session sessions[SESSION_COUNT] __attribute__((section(".network_memory"),aligned(32)));
static uint32_t clock_ms;
static void clear(Session *s) {s->pcb=0;}
static int close_session(Session *s) {
    struct tcp_pcb *pcb=s->pcb;
    if(tcp_close(pcb)!=ERR_OK)return 0;
    tcp_arg(pcb,0);tcp_recv(pcb,0);tcp_sent(pcb,0);tcp_err(pcb,0);
    clear(s);return 1;
}
static void error_cb(void *arg,err_t err) {(void)err;if(arg)clear(arg);}
static void response(Session *s,unsigned status,const char *type,const void *body,size_t length) {
    s->body=body;s->body_length=length;s->body_offset=0;
    s->header_offset=0;s->responding=1;
    s->header_length=snprintf(s->header,sizeof(s->header),
        "HTTP/1.1 %u %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n"
        "Connection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n"
        "X-Frame-Options: SAMEORIGIN\r\n\r\n",status,status==200?"OK":"Error",type,(unsigned long)length);
}
static void reject(Session *s,unsigned status,const char *reason) {
    size_t n=snprintf(s->response,sizeof(s->response),"{\"error\":\"%s\"}",reason);
    response(s,status,"application/json",s->response,n);
}
static void pump(Session *s) {
    if(!s->pcb||!s->responding)return;
    for(unsigned budget=0;budget<2;budget++) {
        const char *p;size_t remaining;
        if(s->header_offset<s->header_length) {
            p=s->header+s->header_offset;remaining=s->header_length-s->header_offset;
        } else {p=s->body+s->body_offset;remaining=s->body_length-s->body_offset;}
        if(!remaining)break;
        unsigned n=remaining>1460?1460:remaining;
        if(n>tcp_sndbuf(s->pcb))n=tcp_sndbuf(s->pcb);
        if(!n||tcp_write(s->pcb,p,n,TCP_WRITE_FLAG_COPY)!=ERR_OK)break;
        s->unacked+=n;
        if(s->header_offset<s->header_length)s->header_offset+=n;else s->body_offset+=n;
    }
    tcp_output(s->pcb);
    if(s->body_offset==s->body_length&&s->header_offset==s->header_length) {
        /* Response bytes have all been copied into lwIP. */
        if(!s->unacked)close_session(s);
    }
}
static err_t sent_cb(void *arg,struct tcp_pcb *pcb,u16_t length) {
    (void)pcb;Session *s=arg;s->activity=clock_ms;
    s->unacked=length>s->unacked?0:s->unacked-length;pump(s);return ERR_OK;
}
static int equal_nocase(const char *a,const char *b,size_t n) {
    while(n--)if(tolower((unsigned char)*a++)!=tolower((unsigned char)*b++))return 0;
    return 1;
}
static const char *header_value(const char *req,const char *key) {
    const char *p=strstr(req,"\r\n");size_t n=strlen(key);
    while(p&&(p+=2)[0]&&p[0]!='\r') {
        if(strlen(p)>=n+1&&equal_nocase(p,key,n)&&p[n]==':') {
            p+=n+1;while(*p==' '||*p=='\t')++p;return p;
        }
        p=strstr(p,"\r\n");
    }
    return 0;
}
static void optional_value(char *out,size_t size,const G4Record *r,const char *key) {
    int64_t value;
    if(!r->valid || (uint32_t)(psu_app_now()-r->ms)>1500 || !g4_record_value(r,key,&value))snprintf(out,size,"null");
    else snprintf(out,size,"%ld",(long)value);
}
static void append_details(Session *s,const PsuSnapshot *v) {
    G4Record battery,charger;g4_record_snapshot(psu_g4(),G4_RECORD_TB,&battery);
    g4_record_snapshot(psu_g4(),G4_RECORD_TC,&charger);
    int64_t bms=0,sample=0;
    if(!g4_record_value(&battery,"bms",&bms)||!g4_record_value(&battery,"sample",&sample)||!bms||!sample)battery.valid=0;
    const char *keys[]={"pack_mv","i_pack_ma","soc_permille","passq_mah","c1_mv","c2_mv","c3_mv","c4_mv","c5_mv"};
    char values[12][24];
    for(unsigned i=0;i<9;i++)optional_value(values[i],sizeof(values[i]),&battery,keys[i]);
    optional_value(values[9],sizeof(values[9]),&charger,"pd_mv");
    optional_value(values[10],sizeof(values[10]),&charger,"pd_ma");
    optional_value(values[11],sizeof(values[11]),&charger,"pd_role");
    size_t n=strlen(s->response);
    if(!n||s->response[n-1]!='}')return;
    snprintf(s->response+n-1,sizeof(s->response)-n+1,
        ",\"stale\":%u,\"current_valid\":%u,\"remote_enabled\":%u,\"meter_ms\":%lu,\"meter_serial\":%lu,\"mode_cc\":%u,"
        "\"mos_centi\":%d,\"temperature_valid\":%u,\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
        "\"battery\":{\"mv\":%s,\"ma\":%s,\"soc\":%s,\"charge_mah\":%s,\"cells\":[%s,%s,%s,%s,%s]},"
        "\"pd\":{\"mv\":%s,\"ma\":%s,\"role\":%s}}",
        v->g0_stale,v->current_valid,psu_remote_enabled(),(unsigned long)v->meter_ms,(unsigned long)v->meter_serial,v->mode_cc,
        v->mos_centi,v->temperature_valid,v->mac[0],v->mac[1],v->mac[2],v->mac[3],v->mac[4],v->mac[5],
        values[0],values[1],values[2],values[3],values[4],values[5],values[6],values[7],values[8],values[9],values[10],values[11]);
}
static void route(Session *s,const char *method,const char *path,const char *body) {
    ++BoardNetDiagnostics[9];
    if(!strcmp(method,"GET")) {
        if(!strcmp(path,"/")){response(s,200,"text/html; charset=utf-8",remote_page,sizeof(remote_page)-1);return;}
        if(!strcmp(path,"/api/state")) {
            PsuSnapshot v;psu_snapshot(&v);
            psu_web_write_snapshot(&v,s->response,sizeof(s->response));append_details(s,&v);
            response(s,200,"application/json",s->response,strlen(s->response));return;
        }
        if(!strcmp(path,"/health")) {
            size_t n=snprintf(s->response,sizeof(s->response),"{\"hostname\":\"LAB-PD-PSU\",\"link\":%lu,\"rx\":%lu,\"tx\":%lu,\"drops\":%lu,\"tx_errors\":%lu,\"max_poll_us\":%lu,\"dhcp_hostname_sent\":%lu}",
                (unsigned long)BoardNetDiagnostics[1],(unsigned long)BoardNetDiagnostics[3],(unsigned long)BoardNetDiagnostics[4],
                (unsigned long)BoardNetDiagnostics[5],(unsigned long)BoardNetDiagnostics[6],(unsigned long)BoardNetDiagnostics[10],
                (unsigned long)BoardNetDiagnostics[12]);
            response(s,200,"application/json",s->response,n);return;
        }
        if(!strcmp(path,"/api/features")||!strcmp(path,"/api/diagnostics")||!strcmp(path,"/api/pd")) {
            int n=psu_web_features_state(path,s->response,sizeof(s->response));
            if(n>0)response(s,200,"application/json",s->response,n);else reject(s,503,"state unavailable");
            return;
        }
        reject(s,404,"not found");return;
    }
    if(strcmp(method,"POST")){reject(s,405,"method not allowed");return;}
    if(!psu_remote_enabled()){reject(s,403,"remote disabled on instrument");return;}
    /* Reject cross-origin browser writes. Direct LAN API clients need no Origin. */
    const char *origin=header_value(s->request,"Origin"),*host=header_value(s->request,"Host");
    if(origin) {
        size_t on=strcspn(origin,"\r\n"),hn=host?strcspn(host,"\r\n"):0;
        if(on!=hn+7||strncmp(origin,"http://",7)||!host||strncmp(origin+7,host,hn)) {
            reject(s,403,"origin mismatch");return;
        }
    }
    const char *type=header_value(s->request,"Content-Type");
    if(!type||strncmp(type,"application/json",16)){reject(s,415,"application/json required");return;}
    if(!strcmp(path,"/api/command")||!strcmp(path,"/api/feature")) {
        char client[24];ipaddr_ntoa_r(&s->pcb->remote_ip,client,sizeof(client));
        psu_web_request(method,path,body,client,s->response,sizeof(s->response));
        response(s,200,"application/json",s->response,strlen(s->response));return;
    }
    reject(s,404,"not found");
}
static void parse(Session *s) {
    char *end=strstr(s->request,"\r\n\r\n");if(!end)return;
    size_t header_len=end+4-s->request;
    const char *cl=header_value(s->request,"Content-Length");unsigned content=0;
    if(header_value(s->request,"Transfer-Encoding")){reject(s,400,"transfer encoding unsupported");return;}
    if(cl) {
        if(!isdigit((unsigned char)*cl)){reject(s,400,"invalid length");return;}
        while(isdigit((unsigned char)*cl)) {content=content*10+(*cl++-'0');if(content>512){reject(s,413,"body too large");return;}}
        if(*cl!='\r'){reject(s,400,"invalid length");return;}
    }
    if(s->used<header_len+content)return;
    if(s->used!=header_len+content){reject(s,400,"pipelining unsupported");return;}
    char method[8],path[80],version[16];
    if(sscanf(s->request,"%7s %79s %15s",method,path,version)!=3||strncmp(version,"HTTP/1.",7)) {
        reject(s,400,"invalid request");return;
    }
    s->parsed=1;route(s,method,path,s->request+header_len);
}
static err_t recv_cb(void *arg,struct tcp_pcb *pcb,struct pbuf *p,err_t err) {
    Session *s=arg;
    if(!p) {
        /* A client may half-close its upload while still reading the reply. */
        if(!s->parsed&&!s->responding){close_session(s);}return ERR_OK;
    }
    if(err!=ERR_OK){pbuf_free(p);return err;}
    tcp_recved(pcb,p->tot_len);s->activity=clock_ms;
    if(!s->parsed&&!s->responding) {
        if(s->used+p->tot_len>=sizeof(s->request))reject(s,413,"request too large");
        else {pbuf_copy_partial(p,s->request+s->used,p->tot_len,0);s->used+=p->tot_len;s->request[s->used]=0;parse(s);}
    }
    pbuf_free(p);pump(s);return ERR_OK;
}
static err_t accept_cb(void *arg,struct tcp_pcb *pcb,err_t err) {
    (void)arg;(void)err;
    for(unsigned i=0;i<SESSION_COUNT;i++)if(!sessions[i].pcb) {
        Session *s=&sessions[i];memset(s,0,sizeof(*s));s->pcb=pcb;s->activity=clock_ms;
        tcp_arg(pcb,s);tcp_recv(pcb,recv_cb);tcp_sent(pcb,sent_cb);tcp_err(pcb,error_cb);
        tcp_nagle_disable(pcb);return ERR_OK;
    }
    tcp_abort(pcb);return ERR_ABRT;
}
void BoardHttp_Init(void) {
    memset(sessions,0,sizeof(sessions));
    struct tcp_pcb *pcb=tcp_new_ip_type(IPADDR_TYPE_V4);
    if(!pcb)return;
    if(tcp_bind(pcb,IP_ANY_TYPE,80)!=ERR_OK){tcp_close(pcb);return;}
    struct tcp_pcb *listener=tcp_listen_with_backlog(pcb,3);
    if(!listener){tcp_close(pcb);return;}
    tcp_accept(listener,accept_cb);
}
void BoardHttp_Poll(uint32_t now) {
    clock_ms=now;
    for(unsigned i=0;i<SESSION_COUNT;i++) {
        Session *s=&sessions[i];if(!s->pcb)continue;
        if(now-s->activity>(s->parsed?15000U:5000U)) {
            struct tcp_pcb *pcb=s->pcb;tcp_arg(pcb,0);tcp_err(pcb,0);tcp_abort(pcb);clear(s);continue;
        }
        pump(s);
    }
}
