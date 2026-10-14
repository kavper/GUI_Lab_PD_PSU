#include "psu_nvstore.h"
#include "psu_app.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint8_t memory[8192], baseline[8192];
static int budget=-1, write_bytes_count, inject_busy, corrupt_reads, read_failure;
static int rd(uint16_t a,uint8_t *p,uint16_t n,void *u)
{
    (void)u;assert((size_t)a+n<=sizeof(memory));memcpy(p,memory+a,n);
    if(read_failure)return -1;
    if(corrupt_reads && a>=4096 && n)p[n-1]^=1;
    return 1;
}
static int wr(uint16_t a,uint8_t *p,uint16_t n,void *u)
{
    (void)u;assert(n<=32 && (a&31)+n<=32 && (size_t)a+n<=sizeof(memory));
    if(inject_busy){--inject_busy;return 0;}
    for(unsigned i=0;i<n;++i) {
        if(budget==0)return -1;
        memory[a+i]=p[i];++write_bytes_count;if(budget>0)--budget;
    }
    return 1;
}
static void ticks(PsuNvStore *s,uint32_t start,uint32_t end)
{ for(uint32_t t=start;t<end;t+=16)psu_nv_tick(s,t); }
int main(void)
{
    PsuNvStore s,boot;uint8_t a[PSU_NV_PAYLOAD],b[PSU_NV_PAYLOAD],out[PSU_NV_PAYLOAD];
    memset(memory,0xff,sizeof(memory));memset(a,0x12,sizeof(a));memset(b,0x34,sizeof(b));
    psu_nv_init(&s,rd,wr,0);assert(!psu_nv_load(&s,out));
    psu_nv_request(&s,a,0);ticks(&s,0,1400);assert(!s.commits);
    inject_busy=5;ticks(&s,1400,4000);assert(s.commits==1 && !s.dirty);
    psu_nv_init(&boot,rd,wr,0);assert(psu_nv_load(&boot,out)&&!memcmp(out,a,sizeof(a)));
    memcpy(baseline,memory,sizeof(memory));
    read_failure=1;psu_nv_init(&boot,rd,wr,0);assert(psu_nv_load(&boot,out)==-1);
    int writes=write_bytes_count;psu_nv_request(&boot,b,0);ticks(&boot,0,20000);
    assert(write_bytes_count==writes);read_failure=0; /* Never replace unread settings with defaults. */
    /* Power loss at every individual programmed byte, including partial commit. */
    for(int cut=0;cut<(int)PSU_NV_RECORD+9;++cut) {
        memcpy(memory,baseline,sizeof(memory));budget=cut;
        psu_nv_init(&s,rd,wr,0);assert(psu_nv_load(&s,out));psu_nv_request(&s,b,0);
        ticks(&s,0,4000);
        psu_nv_init(&boot,rd,wr,0);assert(psu_nv_load(&boot,out));
        assert(!memcmp(out,a,sizeof(a)) || !memcmp(out,b,sizeof(b)));
    }
    budget=-1;memcpy(memory,baseline,sizeof(memory));psu_nv_init(&s,rd,wr,0);assert(psu_nv_load(&s,out));
    psu_nv_request(&s,b,0);ticks(&s,0,4000);assert(s.commits==1);
    int count=write_bytes_count;
    for(uint32_t t=4000;t<30000;t+=16){psu_nv_request(&s,b,t);psu_nv_tick(&s,t);}
    assert(write_bytes_count==count); /* Unchanged settings never wear the EEPROM. */
    /* Header generation is CRC-protected, not merely the payload. */
    memory[4096+8]^=0x80;psu_nv_init(&boot,rd,wr,0);assert(psu_nv_load(&boot,out));assert(!memcmp(out,a,sizeof(a)));
    memcpy(memory,baseline,sizeof(memory));psu_nv_init(&s,rd,wr,0);assert(psu_nv_load(&s,out));
    psu_nv_request(&s,b,0);corrupt_reads=1;ticks(&s,0,4000);assert(!s.commits&&s.errors&&s.dirty);corrupt_reads=0;
    psu_nv_init(&boot,rd,wr,0);assert(psu_nv_load(&boot,out)&&!memcmp(out,a,sizeof(a)));
    /* Latest changes arriving during a commit are saved separately, without losing them. */
    psu_nv_init(&s,rd,wr,0);assert(psu_nv_load(&s,out));psu_nv_request(&s,b,0);
    ticks(&s,0,1800);assert(s.state);psu_nv_request(&s,a,1800);ticks(&s,1800,12000);
    assert(s.commits==2&&!s.dirty);psu_nv_init(&boot,rd,wr,0);assert(psu_nv_load(&boot,out)&&!memcmp(out,a,sizeof(a)));

    psu_app_init();assert(psu_preset_set(0,12345,2345));assert(psu_app_set_limits(7654,1234,PSU_SRC_LCD));
    PsuAppearance appearance={0,2,70,0,2,1};psu_appearance_set(&appearance);
    assert(psu_config_encode(a,sizeof(a)));psu_app_init();assert(psu_config_decode(a,sizeof(a)));
    PsuSnapshot snapshot;psu_snapshot(&snapshot);PsuAppearance restored;psu_appearance_get(&restored);
    assert(snapshot.requested_mv==7654&&snapshot.requested_ma==1234);
    assert(!snapshot.output_requested&&!snapshot.output_confirmed);
    assert(psu_preset_get(0)->voltage_mv==12345&&psu_preset_get(0)->current_ma==2345);
    assert(!memcmp(&appearance,&restored,sizeof(restored)));
    memcpy(b,a,sizeof(b));b[13]=99;assert(!psu_config_decode(b,sizeof(b))); /* Invalid accent: no partial import. */
    assert(psu_config_encode(b,sizeof(b))&&!memcmp(a,b,sizeof(a)));
    /* Restored setpoints must survive the supervisor's initial default readback. */
    for(uint32_t t=16;t<1000;t+=16){psu_sim_tick(t);psu_app_tick(t);}
    psu_snapshot(&snapshot);
    assert(snapshot.requested_mv==7654&&snapshot.requested_ma==1234);
    assert(snapshot.applied_mv==7654&&snapshot.applied_ma==1234);
    assert(!snapshot.output_requested&&!snapshot.output_confirmed);
    puts("EEPROM journal: all byte-level power cuts, CRC, busy/error, wear and config roundtrip passed");return 0;
}
