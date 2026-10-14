#include "psu_nvstore.h"
#include "psu_store.h"
#include <string.h>
#define MAGIC 0x32474643u
#define COMMIT 0x54494d43u
static uint32_t get32(const uint8_t *p) { return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static void put32(uint8_t *p,uint32_t v) { for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8*i)); }
static uint32_t checksum(const uint8_t *p)
{
    uint8_t data[12+PSU_NV_PAYLOAD];
    memcpy(data,p,12); memcpy(data+12,p+20,PSU_NV_PAYLOAD);
    return psu_crc32(data,sizeof(data));
}
static int valid(const uint8_t *p)
{
    return get32(p)==MAGIC && get32(p+4)==PSU_NV_PAYLOAD &&
        get32(p+16)==COMMIT && get32(p+12)==checksum(p);
}
void psu_nv_init(PsuNvStore *s,PsuNvIo read,PsuNvIo write,void *user)
{
    memset(s,0,sizeof(*s));s->read=read;s->write=write;s->user=user;s->active=1;
}
int psu_nv_load(PsuNvStore *s,uint8_t *payload)
{
    int ra=s->read(0,s->record,PSU_NV_RECORD,s->user);
    int rb=s->read(PSU_NV_BANK_SIZE,s->verify,PSU_NV_RECORD,s->user);
    if(ra!=1 || rb!=1){s->ready=0;s->failed=1;++s->errors;return -1;}
    s->ready=1;s->failed=0;
    int a=valid(s->record),b=valid(s->verify);
    if(!a&&!b)return 0;
    s->active=(uint8_t)(b&&(!a||(int32_t)(get32(s->verify+8)-get32(s->record+8))>0));
    const uint8_t *p=s->active?s->verify:s->record;
    s->generation=get32(p+8);memcpy(payload,p+20,PSU_NV_PAYLOAD);
    memcpy(s->saved,payload,PSU_NV_PAYLOAD);memcpy(s->desired,payload,PSU_NV_PAYLOAD);
    s->loaded=1;return 1;
}
void psu_nv_request(PsuNvStore *s,const uint8_t *payload,uint32_t now)
{
    if(!memcmp(s->desired,payload,PSU_NV_PAYLOAD)&&(s->loaded||s->dirty))return;
    memcpy(s->desired,payload,PSU_NV_PAYLOAD);
    if(!s->dirty)s->first_dirty=now;
    s->changed_at=now;s->dirty=(uint8_t)(!s->loaded||memcmp(s->desired,s->saved,PSU_NV_PAYLOAD)!=0);
}
void psu_nv_tick(PsuNvStore *s,uint32_t now)
{
    if(!s->ready)return;
    int rc;
    uint16_t base=(uint16_t)(s->target*PSU_NV_BANK_SIZE),n;
    if((int32_t)(now-s->due)<0)return;
    if(!s->state) {
        if(!s->dirty || ((uint32_t)(now-s->changed_at)<1500u && (uint32_t)(now-s->first_dirty)<10000u) ||
            (s->commits && (uint32_t)(now-s->last_commit)<5000u))return;
        memset(s->record,0,sizeof(s->record));
        put32(s->record,MAGIC);put32(s->record+4,PSU_NV_PAYLOAD);
        put32(s->record+8,s->generation+1u);memcpy(s->record+20,s->desired,PSU_NV_PAYLOAD);
        put32(s->record+12,checksum(s->record));
        s->target=(uint8_t)(1u-s->active);s->state=1;s->offset=0;return;
    }
    if(s->state==1)rc=s->write((uint16_t)(base+16),s->record+16,4,s->user);
    else if(s->state==2) {
        n=(uint16_t)(32u-(s->offset&31u));
        if(n>PSU_NV_RECORD-s->offset)n=(uint16_t)(PSU_NV_RECORD-s->offset);
        rc=s->write((uint16_t)(base+s->offset),s->record+s->offset,n,s->user);
        if(rc==1)s->offset=(uint16_t)(s->offset+n);
    } else if(s->state==3) {
        n=(uint16_t)(PSU_NV_RECORD-s->offset);if(n>32)n=32;
        rc=s->read((uint16_t)(base+s->offset),s->verify+s->offset,n,s->user);
        if(rc==1)s->offset=(uint16_t)(s->offset+n);
    } else if(s->state==4) {
        put32(s->record+16,COMMIT);rc=s->write((uint16_t)(base+16),s->record+16,4,s->user);
    } else {
        rc=s->read((uint16_t)(base+16),s->verify+16,4,s->user);
        if(rc==1&&!valid(s->verify))rc=-1;
    }
    if(rc<0) { ++s->errors;s->failed=1;s->state=0;s->due=now+5000u;return; }
    if(!rc)return;
    s->due=now+6u; /* 24AA64 maximum internal page program cycle: 5 ms. */
    if(s->state==1)s->state=2;
    else if(s->state==2&&s->offset==PSU_NV_RECORD){s->state=3;s->offset=0;}
    else if(s->state==3&&s->offset==PSU_NV_RECORD) {
        if(memcmp(s->record,s->verify,PSU_NV_RECORD)){++s->errors;s->failed=1;s->state=0;s->due=now+5000u;}
        else s->state=4;
    } else if(s->state==4)s->state=5;
    else if(s->state==5) {
        s->active=s->target;s->generation=get32(s->record+8);s->loaded=1;s->failed=0;++s->commits;
        memcpy(s->saved,s->record+20,PSU_NV_PAYLOAD);
        s->dirty=(uint8_t)(memcmp(s->saved,s->desired,PSU_NV_PAYLOAD)!=0);
        s->first_dirty=now;s->last_commit=now;s->state=0;
    }
}
