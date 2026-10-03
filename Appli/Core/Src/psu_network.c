#include "psu_network.h"
#include "ethernetif.h"
#include "web_control.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/dhcp.h"
#include "lwip/mem.h"
#include "lwip/tcp.h"
#include "lwip/ip4_addr.h"
#include "netif/ethernet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct netif netif0;
static uint32_t network_started_at;
static char address_text[20]="NO LINK";
static uint8_t network_initialized;
static uint8_t dhcp_active;
static uint8_t previous_link_up;

static void clear_ipv4_address(void)
{
 ip4_addr_t zero;ip4_addr_set_zero(&zero);netif_set_addr(&netif0,&zero,&zero,&zero);
}

static const char page[]=
"<!doctype html><html><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
"<title>LAB PD PSU</title><style>"
":root{color-scheme:dark;--bg:#07131b;--card:#0e1d29;--line:#29465b;--cyan:#2ac7d9;--text:#eef4fb;--muted:#93a3b8;--green:#36d789;--red:#ff5c68}"
"*{box-sizing:border-box}body{margin:0;background:linear-gradient(145deg,#061019,#0b1b26);font:15px system-ui;color:var(--text)}"
"header{height:76px;border-bottom:1px solid var(--line);display:flex;align-items:center;padding:0 30px;gap:28px}h1{font-size:24px;margin:0}"
".mode{background:var(--cyan);color:#05131a;padding:9px 18px;border-radius:9px}.spacer{flex:1}.status{color:var(--green);font-weight:700}"
"main{max-width:1180px;margin:auto;padding:24px;display:grid;gap:18px}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:14px}"
".card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:18px}.label{color:var(--muted);font-size:12px;letter-spacing:.08em}.value{font-size:34px;margin-top:7px}"
".control{display:grid;grid-template-columns:1fr 1fr;gap:16px}.set{display:flex;gap:10px;align-items:end}input{width:100%;background:#091721;border:1px solid var(--line);border-radius:9px;padding:13px;color:var(--text);font-size:20px}"
"button{border:1px solid var(--line);border-radius:9px;padding:13px 18px;background:#142536;color:var(--text);font-weight:700;cursor:pointer}button.primary{background:var(--cyan);color:#05131a;border:0}button.danger{background:var(--red);border:0}"
"nav{display:flex;gap:8px}.tab{padding:9px 14px}.panel{display:none}.panel.active{display:block}table{width:100%;border-collapse:separate;border-spacing:0 8px}th{text-align:left;color:var(--muted);font-size:12px}td{background:#102231;padding:12px}td:first-child{border-radius:9px 0 0 9px}td:last-child{border-radius:0 9px 9px 0}"
".seqbar{display:flex;gap:9px;align-items:center;flex-wrap:wrap;margin-bottom:12px}.seqbar select{background:#142536;color:var(--text);border:1px solid var(--line);border-radius:9px;padding:12px}.seqin{font-size:14px;padding:8px;min-width:72px;background:#091721;color:var(--text);border:1px solid var(--line);border-radius:7px}.running td{outline:1px solid #ffae42;color:#ffbf66}.run{background:var(--green);color:#07131b}.stop{background:var(--red)}"
"@media(max-width:800px){.grid{grid-template-columns:1fr 1fr}.control{grid-template-columns:1fr}header{padding:0 14px}.value{font-size:27px}}"
"</style></head><body><header><h1>LAB PD PSU</h1><span class=mode id=mode>CV</span><div class=spacer></div><span id=link class=status>CONNECTED</span></header>"
"<main><div class=grid><div class=card><div class=label>MEASURED VOLTAGE</div><div class=value id=vout>0.000 V</div></div>"
"<div class=card><div class=label>MEASURED CURRENT</div><div class=value id=iout>0.000 A</div></div>"
"<div class=card><div class=label>INPUT VOLTAGE</div><div class=value id=vin>0.000 V</div></div>"
"<div class=card><div class=label>OUTPUT POWER</div><div class=value id=power>0.00 W</div></div></div>"
"<div class='card control'><div><div class=label>VOLTAGE SET</div><div class=set><input id=vset type=number min=0 max=27 step=.001 value=12.00 oninput=limitInput(this,0,27) onblur=normaliseMain()><span>V</span></div></div>"
"<div><div class=label>CURRENT LIMIT</div><div class=set><input id=iset type=number min=0 max=5 step=.001 value=2.000 oninput=limitInput(this,0,5) onblur=normaliseMain()><span>A</span></div></div>"
"<button class=primary onclick=apply()>APPLY SETPOINTS</button><button id=out onclick=output()>OUTPUT OFF</button></div>"
"<nav><button class=tab onclick=show('presets')>PRESETS</button><button class=tab onclick=show('sequence')>SEQUENCER</button><button class=tab onclick=show('diag')>DIAGNOSTICS</button><span id=commandStatus class=status>READY</span></nav>"
"<section id=presets class='card panel active'><h2>Quick presets</h2><div id=presetList></div></section>"
"<section id=sequence class='card panel'><h2>Output sequencer</h2><div class=seqbar><button onclick=seqCount(-1)>REMOVE LAST</button><button onclick=seqCount(1)>ADD STEP</button><label>LOOP <select id=seqLoop onchange=seqLoopSet()><option value=1>ONCE</option><option value=2>2X</option><option value=3>3X</option><option value=5>5X</option><option value=255>INFINITE</option></select></label><button id=seqRun onclick=seqRunSet()>RUN</button><span id=seqStatus class=status>IDLE</span></div><table><thead><tr><th>STEP</th><th>VOLTAGE [V]</th><th>CURRENT [A]</th><th>TIME [S]</th><th>SLEW [V/S]</th><th>STATE</th><th></th></tr></thead><tbody id=steps></tbody></table></section>"
"<section id=diag class='card panel'><h2>Controller</h2><pre id=raw></pre></section></main>"
"<script>let state={output:0},seq={count:1,running:0,steps:[]};const f=(n,d=3)=>(n||0).toFixed(d),clamp=(n,a,b)=>Math.min(b,Math.max(a,Number.isFinite(n)?n:a)),vf=n=>n<10?f(n,3):f(n,2);function limitInput(e,a,b){if(e.value==='')return;let n=Number(e.value);if(!Number.isFinite(n))return;if(n<a)e.value=a;else if(n>b)e.value=b}function normaliseMain(){vset.value=vf(clamp(+vset.value,0,27));iset.value=f(clamp(+iset.value,0,5),3)}async function poll(){try{let r=await fetch('/api/state');state=await r.json();vout.textContent=vf(state.vout/1000)+' V';iout.textContent=f(Math.max(0,state.iout)/1e6)+' A';vin.textContent=vf(state.vin/1000)+' V';power.textContent=f(state.vout*Math.max(0,state.iout)/1e9,2)+' W';if(document.activeElement!==vset)vset.value=vf(state.vset/1000);if(document.activeElement!==iset)iset.value=f(state.iset/1000);mode.textContent=state.mode?'CC':'CV';link.textContent=state.connected?'CONTROLLER ONLINE':'CONTROLLER OFFLINE';out.textContent=state.output?'OUTPUT ON':'OUTPUT OFF';out.className=state.output?'danger':'';raw.textContent=JSON.stringify(state,null,2)}catch(e){link.textContent='NETWORK ERROR'}}"
"async function apply(){normaliseMain();let v=Math.round(clamp(+vset.value,0,27)*1000),i=Math.round(clamp(+iset.value,0,5)*1000);let r=await fetch('/api/set',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'v='+v+'&i='+i});let a=await r.json();vset.value=vf(a.voltage_mv/1000);iset.value=f(a.current_ma/1000);commandStatus.textContent=a.controller?'SETPOINT APPLIED':'SETPOINT SAVED - CONTROLLER OFFLINE'}async function output(){let r=await fetch('/api/output',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'enabled='+(state.output?0:1)});let a=await r.json();commandStatus.textContent=a.controller?'OUTPUT COMMAND SENT':'QUEUED - CONTROLLER OFFLINE'}"
"function show(id){document.querySelectorAll('.panel').forEach(x=>x.classList.remove('active'));document.getElementById(id).classList.add('active')}"
"presetList.innerHTML=['5 V / 1 A','12 V / 2 A','20 V / 3 A'].map((x,n)=>'<button onclick=\"vset.value=[5,12,20]['+n+'];iset.value=[1,2,3]['+n+'];apply()\">'+x+'</button>').join(' ');"
"async function pollSeq(){try{let r=await fetch('/api/sequence'),n=await r.json();seq=n;if(!document.querySelector('#steps input:focus'))renderSeq();seqLoop.value=n.loop;seqRun.textContent=n.running?'STOP':'RUN';seqRun.className=n.running?'stop':'run';seqStatus.textContent=n.running?'RUNNING STEP '+(n.runningStep+1)+' / '+n.count:['IDLE','RUNNING','COMPLETE','STOPPED','NO ENABLED STEPS','ABORTED'][n.state]||'IDLE'}catch(e){seqStatus.textContent='NETWORK ERROR'}}"
"function renderSeq(){steps.innerHTML=seq.steps.map((s,n)=>'<tr class=\"'+(seq.running&&seq.runningStep==n?'running':'')+'\"><td>'+(n+1)+'</td><td><input class=seqin id=sv'+n+' type=number min=0 max=27 step=.001 oninput=\"limitInput(this,0,27)\" value=\"'+f(s.v/1000,3)+'\"></td><td><input class=seqin id=si'+n+' type=number min=0 max=5 step=.001 oninput=\"limitInput(this,0,5)\" value=\"'+f(s.i/1000,3)+'\"></td><td><input class=seqin id=st'+n+' type=number min=.1 max=3600 step=.001 oninput=\"limitInput(this,.1,3600)\" value=\"'+f(s.t/60,3)+'\"></td><td><input class=seqin id=ss'+n+' type=number min=0 max=100 step=.001 oninput=\"limitInput(this,0,100)\" value=\"'+f(s.s/1000,3)+'\"></td><td><button onclick=seqToggle('+n+')>'+(s.e?'ENABLED':'SKIPPED')+'</button></td><td><button onclick=seqSave('+n+')>SAVE</button></td></tr>').join('')}"
"async function postSeq(path,body){let r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body});let a=await r.json();commandStatus.textContent=a.ok?'SEQUENCER UPDATED':'SEQUENCER BUSY';await pollSeq()}async function seqSave(n){let s=seq.steps[n],v=clamp(+document.getElementById('sv'+n).value,0,27),i=clamp(+document.getElementById('si'+n).value,0,5),t=clamp(+document.getElementById('st'+n).value,.1,3600),sr=clamp(+document.getElementById('ss'+n).value,0,100);await postSeq('/api/sequence/step','idx='+n+'&v='+Math.round(v*1000)+'&i='+Math.round(i*1000)+'&t='+Math.round(t*60)+'&s='+Math.round(sr*1000)+'&e='+(s.e?1:0))}function seqToggle(n){seq.steps[n].e=seq.steps[n].e?0:1;seqSave(n)}function seqCount(d){postSeq('/api/sequence/count','count='+(seq.count+d))}function seqLoopSet(){postSeq('/api/sequence/loop','loop='+seqLoop.value)}function seqRunSet(){postSeq('/api/sequence/run','run='+(seq.running?0:1))}"
"setInterval(poll,500);setInterval(pollSeq,600);poll();pollSeq();</script></body></html>";

static uint32_t form_value(const char* body,const char* key,uint32_t fallback){const char*p=strstr(body,key);if(!p)return fallback;long value=strtol(p+strlen(key),0,10);return value<0?0U:(uint32_t)value;}

typedef struct
{
 char data[1536];
 char json[2048];
 char header[192];
 const char* response;
 u16_t length;
 u16_t header_length;
 u16_t header_offset;
 uint32_t response_length;
 uint32_t response_offset;
 uint8_t responding;
} HttpRequest;

static err_t http_sent(void* arg,struct tcp_pcb* pcb,u16_t length);
static err_t http_poll(void* arg,struct tcp_pcb* pcb);

static err_t http_send_pending(struct tcp_pcb* pcb,HttpRequest* request)
{
 while(request->header_offset<request->header_length)
 {
  u16_t available=tcp_sndbuf(pcb);if(!available)break;
  u16_t remaining=(u16_t)(request->header_length-request->header_offset);u16_t chunk=remaining<available?remaining:available;if(chunk>1024U)chunk=1024U;
  err_t result=tcp_write(pcb,request->header+request->header_offset,chunk,TCP_WRITE_FLAG_COPY);if(result!=ERR_OK)break;request->header_offset=(u16_t)(request->header_offset+chunk);
 }
 while(request->header_offset==request->header_length&&request->response_offset<request->response_length)
 {
  u16_t available=tcp_sndbuf(pcb);if(!available)break;
  uint32_t remaining=request->response_length-request->response_offset;u16_t chunk=(u16_t)(remaining<available?remaining:available);if(chunk>1024U)chunk=1024U;
  err_t result=tcp_write(pcb,request->response+request->response_offset,chunk,TCP_WRITE_FLAG_COPY);if(result!=ERR_OK)break;request->response_offset+=chunk;
 }
 tcp_output(pcb);
 if(request->header_offset==request->header_length&&request->response_offset==request->response_length)
 {
  tcp_arg(pcb,NULL);tcp_sent(pcb,NULL);tcp_poll(pcb,NULL,0);
  err_t closeResult=tcp_close(pcb);
  if(closeResult!=ERR_OK)
  {
   tcp_arg(pcb,request);tcp_sent(pcb,http_sent);tcp_poll(pcb,http_poll,2U);
   return ERR_OK;
  }
  mem_free(request);
  return ERR_OK;
 }
 return ERR_OK;
}

static err_t http_sent(void* arg,struct tcp_pcb* pcb,u16_t length)
{
 (void)length;return http_send_pending(pcb,(HttpRequest*)arg);
}

static err_t http_poll(void* arg,struct tcp_pcb* pcb)
{
 return http_send_pending(pcb,(HttpRequest*)arg);
}

static void begin_response(struct tcp_pcb* pcb,HttpRequest* request,const char* type,const char* body)
{
 request->responding=1U;request->response=body;request->response_length=(uint32_t)strlen(body);request->response_offset=0U;request->header_offset=0U;
 request->header_length=(u16_t)snprintf(request->header,sizeof(request->header),"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: %lu\r\n\r\n",type,(unsigned long)request->response_length);
 tcp_sent(pcb,http_sent);tcp_poll(pcb,http_poll,2U);http_send_pending(pcb,request);
}

static void http_error(void*arg,err_t err)
{
 (void)err;
 if(arg)mem_free(arg);
}

static err_t http_recv(void*arg,struct tcp_pcb*pcb,struct pbuf*p,err_t err)
{
 HttpRequest*request=(HttpRequest*)arg;
 if(err!=ERR_OK){if(p)pbuf_free(p);return err;}
 if(!p){if(request)mem_free(request);tcp_arg(pcb,NULL);tcp_close(pcb);return ERR_OK;}
 if(!request){pbuf_free(p);tcp_abort(pcb);return ERR_ABRT;}
 if(request->responding){tcp_recved(pcb,p->tot_len);pbuf_free(p);return ERR_OK;}
 u16_t room=(u16_t)(sizeof(request->data)-1U-request->length);
 u16_t copy=p->tot_len<room?p->tot_len:room;
 pbuf_copy_partial(p,request->data+request->length,copy,0);
 request->length=(u16_t)(request->length+copy);request->data[request->length]=0;
 tcp_recved(pcb,p->tot_len);pbuf_free(p);
 if(copy==0U){begin_response(pcb,request,"text/plain","REQUEST TOO LARGE");return ERR_OK;}

 char*headerEnd=strstr(request->data,"\r\n\r\n");
 if(!headerEnd)return ERR_OK;
 uint32_t contentLength=0U;char*lengthHeader=strstr(request->data,"Content-Length:");
 if(lengthHeader)contentLength=(uint32_t)strtoul(lengthHeader+15,NULL,10);
 char*body=headerEnd+4;
 if((uint32_t)request->length<(uint32_t)(body-request->data)+contentLength)return ERR_OK;

 WebTelemetry t;char*json=request->json;WebControl_GetTelemetry(&t);snprintf(json,sizeof(request->json),"{\"ok\":false}");
 if(!strncmp(request->data,"GET /api/state",14))
  snprintf(json,sizeof(request->json),"{\"vin\":%lu,\"vout\":%lu,\"vset\":%lu,\"iset\":%lu,\"iout\":%ld,\"mosTemp\":%d,\"pcbTemp\":%d,\"mode\":%u,\"connected\":%u,\"output\":%u}",(unsigned long)t.vin_mv,(unsigned long)t.vout_mv,(unsigned long)t.set_voltage_mv,(unsigned long)t.set_current_ma,(long)t.iout_ua,t.mos_deci_c,t.pcb_deci_c,t.mode,t.connected,t.output_on);
 else if(!strncmp(request->data,"POST /api/set",13))
 {
  uint32_t v=form_value(body,"v=",12000),i=form_value(body,"i=",2000);if(v>27000)v=27000;if(i>5000)i=5000;
  WebControl_RequestLimits(v,i);
  snprintf(json,sizeof(request->json),"{\"ok\":true,\"voltage_mv\":%lu,\"current_ma\":%lu,\"controller\":%u}",(unsigned long)v,(unsigned long)i,t.connected);
 }
 else if(!strncmp(request->data,"POST /api/output",16))
 {
  uint8_t enabled=(uint8_t)form_value(body,"enabled=",0);WebControl_RequestOutput(enabled);
  snprintf(json,sizeof(request->json),"{\"ok\":true,\"enabled\":%u,\"controller\":%u}",enabled?1U:0U,t.connected);
 }
 else if(!strncmp(request->data,"GET /api/sequence ",18))
 {
  WebSequenceSnapshot s;WebControl_GetSequence(&s);size_t used=(size_t)snprintf(json,sizeof(request->json),"{\"count\":%u,\"loop\":%u,\"running\":%u,\"runningStep\":%u,\"completedLoops\":%u,\"state\":%u,\"revision\":%lu,\"steps\":[",s.count,s.loop_mode,s.running,s.running_step,s.completed_loops,s.state,(unsigned long)s.revision);
  for(uint8_t i=0;i<s.count&&used<sizeof(request->json);++i)used+=(size_t)snprintf(json+used,sizeof(request->json)-used,"%s{\"v\":%lu,\"i\":%lu,\"t\":%lu,\"s\":%lu,\"e\":%u}",i?",":"",(unsigned long)s.steps[i].voltage_mv,(unsigned long)s.steps[i].current_ma,(unsigned long)s.steps[i].duration_ticks,(unsigned long)s.steps[i].slew_mv_per_second,s.steps[i].enabled);
  if(used<sizeof(request->json)-2U){json[used++]=']';json[used++]='}';json[used]=0;}
 }
 else if(!strncmp(request->data,"POST /api/sequence/step ",24))
 {
  WebSequenceStep s;s.voltage_mv=form_value(body,"v=",0);s.current_ma=form_value(body,"i=",0);s.duration_ticks=form_value(body,"t=",60);s.slew_mv_per_second=form_value(body,"s=",0);s.enabled=(uint8_t)form_value(body,"e=",1);
  uint8_t index=(uint8_t)form_value(body,"idx=",0);if(s.voltage_mv>27000U)s.voltage_mv=27000U;if(s.current_ma>5000U)s.current_ma=5000U;if(s.duration_ticks<6U)s.duration_ticks=6U;if(s.duration_ticks>216000U)s.duration_ticks=216000U;if(s.slew_mv_per_second>100000U)s.slew_mv_per_second=100000U;
  WebControl_RequestSequenceStep(index,&s);snprintf(json,sizeof(request->json),"{\"ok\":true}");
 }
 else if(!strncmp(request->data,"POST /api/sequence/count ",25))
 {
  uint8_t count=(uint8_t)form_value(body,"count=",1);if(count<1U)count=1U;if(count>12U)count=12U;WebControl_RequestSequenceCount(count);snprintf(json,sizeof(request->json),"{\"ok\":true}");
 }
 else if(!strncmp(request->data,"POST /api/sequence/loop ",24))
 {
  WebControl_RequestSequenceLoop((uint8_t)form_value(body,"loop=",1));snprintf(json,sizeof(request->json),"{\"ok\":true}");
 }
 else if(!strncmp(request->data,"POST /api/sequence/run ",23))
 {
  WebControl_RequestSequenceRun((uint8_t)form_value(body,"run=",0));snprintf(json,sizeof(request->json),"{\"ok\":true}");
 }
 uint8_t isApi=(uint8_t)(!strncmp(request->data,"GET /api/state",14)||!strncmp(request->data,"GET /api/sequence ",18)||!strncmp(request->data,"POST /api/",10));
 if(isApi)begin_response(pcb,request,"application/json",json);
 else begin_response(pcb,request,"text/html; charset=utf-8",page);
 return ERR_OK;
}
static err_t http_accept(void*arg,struct tcp_pcb*pcb,err_t err){(void)arg;(void)err;HttpRequest*r=(HttpRequest*)mem_calloc(1,sizeof(HttpRequest));if(!r){tcp_abort(pcb);return ERR_ABRT;}tcp_arg(pcb,r);tcp_err(pcb,http_error);tcp_recv(pcb,http_recv);return ERR_OK;}
static void web_init(void){struct tcp_pcb*pcb=tcp_new_ip_type(IPADDR_TYPE_V4);if(pcb&&tcp_bind(pcb,IP_ANY_TYPE,80)==ERR_OK){pcb=tcp_listen(pcb);tcp_accept(pcb,http_accept);}}

void PSU_Network_Init(void)
{
 ip4_addr_t zero;
 ip4_addr_set_zero(&zero);
 lwip_init();
 if(netif_add(&netif0,&zero,&zero,&zero,0,ethernetif_init,ethernet_input)==NULL || !ethernetif_is_ready())
 {
  snprintf(address_text,sizeof(address_text),"ETH INIT ERROR");
  network_initialized=0U;
  return;
 }
 network_initialized=1U;
 netif_set_default(&netif0);
 ethernet_link_check_state(&netif0);
 previous_link_up=netif_is_link_up(&netif0)?1U:0U;
 if(netif_is_link_up(&netif0))
 {
  netif_set_up(&netif0);
  if(dhcp_start(&netif0)==ERR_OK)dhcp_active=1U;
 }
 network_started_at=sys_now();
 web_init();
}
void PSU_Network_Process(void)
{
 if(!network_initialized)return;
 ethernetif_input(&netif0);
 sys_check_timeouts();
 static uint32_t linkTick;
 if(sys_now()-linkTick>100)
 {
  linkTick=sys_now();
  ethernet_link_check_state(&netif0);
  const uint8_t linkUp=netif_is_link_up(&netif0)?1U:0U;
  if(linkUp!=previous_link_up)
  {
   previous_link_up=linkUp;
   if(linkUp)
   {
    if(dhcp_active)dhcp_stop(&netif0);
    dhcp_active=0U;clear_ipv4_address();netif_set_up(&netif0);
    if(dhcp_start(&netif0)==ERR_OK)dhcp_active=1U;
    network_started_at=sys_now();
   }
   else
   {
    if(dhcp_active)dhcp_stop(&netif0);
    dhcp_active=0U;clear_ipv4_address();
   }
  }
  else if(linkUp&&!dhcp_active&&ip4_addr_isany_val(*netif_ip4_addr(&netif0)))
  {
   if(dhcp_start(&netif0)==ERR_OK)dhcp_active=1U;
   network_started_at=sys_now();
  }
 }
 if(netif_is_up(&netif0)&&dhcp_active&&ip4_addr_isany_val(*netif_ip4_addr(&netif0))&&sys_now()-network_started_at>20000U)
 {
  /* A cable replug can leave a DHCP transaction waiting for an obsolete
     offer. Restart discovery instead of assigning an unrelated static IP. */
  dhcp_stop(&netif0);dhcp_active=0U;clear_ipv4_address();
  if(dhcp_start(&netif0)==ERR_OK)dhcp_active=1U;
  network_started_at=sys_now();
 }
 if(!ip4_addr_isany_val(*netif_ip4_addr(&netif0)))snprintf(address_text,sizeof(address_text),"%s",ip4addr_ntoa(netif_ip4_addr(&netif0)));else snprintf(address_text,sizeof(address_text),netif_is_link_up(&netif0)?"DHCP...":"NO LINK");
}
const char* PSU_Network_Address(void){return address_text;}
