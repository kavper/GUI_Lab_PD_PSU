/* Exercise the actual HTTP close helper against the bundled lwIP allocator.
 * A successful close revokes access, even if a particular PCB is only deferred. */
#include <assert.h>
#include "lwip/tcp.h"
#include "lwip/init.h"
#include "lwip/priv/tcp_priv.h"
static struct tcp_pcb *revoked;
static int fail_close;
static err_t checked_close(struct tcp_pcb *p) {
    if(fail_close)return ERR_MEM;
    err_t result=tcp_close(p);
    if(result==ERR_OK)revoked=p;
    return result;
}
static void checked_arg(struct tcp_pcb *p,void *arg){assert(p!=revoked);tcp_arg(p,arg);}
static void checked_recv(struct tcp_pcb *p,tcp_recv_fn fn){assert(p!=revoked);tcp_recv(p,fn);}
static void checked_sent(struct tcp_pcb *p,tcp_sent_fn fn){assert(p!=revoked);tcp_sent(p,fn);}
static void checked_err(struct tcp_pcb *p,tcp_err_fn fn){assert(p!=revoked);tcp_err(p,fn);}
#define tcp_close checked_close
#define tcp_arg checked_arg
#define tcp_recv checked_recv
#define tcp_sent checked_sent
#define tcp_err checked_err
#ifndef HTTP_IMPLEMENTATION
#define HTTP_IMPLEMENTATION "../Board/board_http.c"
#endif
#include HTTP_IMPLEMENTATION
#undef tcp_close
#undef tcp_arg
#undef tcp_recv
#undef tcp_sent
#undef tcp_err
unsigned char BoardNetHeap[MEM_SIZE+128];
volatile uint32_t BoardNetDiagnostics[13];
u32_t sys_now(void){return 0;}
int main(void){
    lwip_init();psu_app_init();Session s={0};
    s.pcb=tcp_new();assert(s.pcb);assert(s.pcb->state==CLOSED);
    tcp_arg(s.pcb,&s);tcp_recv(s.pcb,recv_cb);tcp_sent(s.pcb,sent_cb);tcp_err(s.pcb,error_cb);
    assert(close_session(&s));assert(!s.pcb); /* CLOSED frees immediately. */
    revoked=0;s.pcb=tcp_new();assert(s.pcb);fail_close=1;
    assert(!close_session(&s));assert(s.pcb&&s.pcb->callback_arg==&s);
    assert(s.pcb->recv==recv_cb&&s.pcb->sent==sent_cb&&s.pcb->errf==error_cb);
    fail_close=0;assert(close_session(&s));assert(!s.pcb);
    puts("PASS HTTP lifetime: immediate PCB free has no later access; failed close restores all callbacks for retry");
    return 0;
}
