#include "psu_app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static char reply[2048];
static void request(const char *body,const char *client) {
    assert(psu_web_request("POST","/api/command",body,client,reply,sizeof(reply))>0);
}
int main(void) {
    psu_app_init();
    const char *invalid[]={"{}","{\"output\":}","{\"output\":-1}","{\"output\":2}",
        "{\"output\":1.5}","{\"output\":4294967296}","{\"output\":01}",
        "{\"output\":0,\"output\":1}","{\"unknown\":0}","{\"voltage_mv\":1000}",
        "{\"output\":0}garbage","{\"voltage_mv\":1000,\"current_ma\":100,\"output\":1}"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++) {
        request(invalid[i],"192.168.0.10");assert(strstr(reply,"invalid_command"));
    }
    request(" { \"output\" : 0 } \r\n","192.168.0.10");assert(!strstr(reply,"invalid_command"));
    request("{\"output\":0}","192.168.0.11");assert(strstr(reply,"lease"));
    psu_app_tick(10001);
    request("{\"output\":0}","192.168.0.11");assert(!strstr(reply,"lease"));
    request("{\"current_ma\":100, \"voltage_mv\":1000}","192.168.0.11");assert(!strstr(reply,"invalid_command"));
    uint8_t ip[4]={192,168,0,10};psu_net_configured(1);psu_net_up();assert(psu_net_bound(ip,0,0));
    psu_remote_enable(0);PsuSnapshot s;psu_snapshot(&s);assert(s.net_state==PSU_NET_BOUND&&s.ethernet_configured);
    assert(psu_web_request("POST","/api/command","{\"output\":1}","192.168.0.11",reply,sizeof(reply))==0);
    psu_remote_enable(1);psu_web_write_state(reply,sizeof(reply));assert(strstr(reply,"192.168.0.10"));
    puts("PASS web commands: strict JSON, overflow/duplicates, lease expiry, remote disable preserves DHCP address");
    return 0;
}
