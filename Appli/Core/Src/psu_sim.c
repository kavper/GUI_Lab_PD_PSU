#include "psu_app.h"
#if defined(SIMULATOR) || defined(PSU_SIMULATOR)
#include <stdio.h>
#include <string.h>

/* Demo controller uses the exact host parser, not fabricated UI snapshots.
   Compiled out on hardware. Fixed readings exercise every telemetry page. */
static void receive(const char *line,uint32_t now) {
  g4_rx_bytes(psu_g4(),(const uint8_t*)line,strlen(line),now);
}
void psu_sim_tick(uint32_t now) {
  static uint8_t initialized,on,permit=1,remote,role=1;
  static uint32_t last,mv=12000,ma=1500;
  char cmd[100],line[1536];
  if(!initialized){initialized=1;psu_app_set_g4_uart(1);psu_g4()->need_session_init=1;}
  g4_process(psu_g4(),now);
  if(g4_pop_tx(psu_g4(),cmd,sizeof(cmd),now)) {
    unsigned v,vd,i,id;
    if(sscanf(cmd,"SET V=%u.%u I=%u.%u",&v,&vd,&i,&id)==4){mv=v*1000+vd;ma=i*1000+id;receive("OK SET\r\n",now);}
    else if(!strncmp(cmd,"ON\r",3)){on=permit;receive("OK ON\r\n",now);}
    else if(!strncmp(cmd,"OFF\r",4)){on=0;receive("OK OFF\r\n",now);}
    else if(!strncmp(cmd,"PERMIT ",7)){permit=cmd[7]=='1';if(!permit)on=0;receive("OK PERMIT\r\n",now);}
    else if(!strncmp(cmd,"REMOTE ",7)){remote=cmd[7]=='1';receive(remote?"OK REMOTE\r\n":"OK LOCAL\r\n",now);}
    else if(!strncmp(cmd,"USB ",4)){role=strstr(cmd,"SOURCE")?2:1;receive("OK USB\r\n",now);}
    else if(!strncmp(cmd,"TEL",3))receive("OK TEL 500 ms\r\n",now);
    else if(!strncmp(cmd,"BMS",3))receive("OK BMS\r\n",now);
    else if(!strncmp(cmd,"CLR",3))receive("OK CLR\r\n",now);
    else if(!strncmp(cmd,"HELP",4))receive("HELP ON OFF SET CLR STATUS TEL USB PERMIT REMOTE\r\n",now);
    last=0;
  }
  if(last && now-last<500)return;
  last=now;
  snprintf(line,sizeof(line),
    "T vin_mv=20000 vout_mv=%lu iout_ma=%u i_buck_ma=%u i_boost_ma=5 set_mv=%lu ilim_ma=%lu duty_a_x10=%u duty_c_x10=0 ucc_a=0 ucc_c=0 run=%u mode=%s fault=0 pd=1 pd_mv=20000 pd_ma=3000 pd_mw=60000 permit=%u rem_sense=%u g0=%u g0_out=%u g0_want=%u g0_ctrl=%u g0_kill=0 g0_outoff=%u g0_fault=0 g0_vout_mv=%lu g0_iout_ma=%u vpre_req_mv=%lu vpre_cmd_mv=%lu reg_ok=1 stage_en=%u ps_en=%u flt=0 hold_ms=0 ps_err=0 g0_rx=12000 g0_tlm=400 g0_age_ms=80 g0_err=0 g0_uart=0 pm_st=2 fmt=0\r\n",
    (unsigned long)(on?mv+1500:0),on?420:0,on?410:0,(unsigned long)mv,(unsigned long)ma,on?675:0,on,on?"CV":"IDLE",permit,remote,on,on,on,on?9:0,on?0:1,(unsigned long)(on?mv+10:0),on?410:0,(unsigned long)(on?mv+1500:0),(unsigned long)(on?mv+1500:0),on,on);
  receive(line,now);
  receive("TB bms=1 cfg=1 st=4 fault=0x0 alert=0 alarm=0x0 sa=0x0 sb=0x0 sc=0x0 fet=0x5 manuf=0x50 init_step=12 vcell_rb=0x0017 batt=0x0080 cfg_fail=0 chg=1 dsg=1 fets=1 series=4 c1_mv=4001 c2_mv=4002 c3_mv=4003 c4_mv=-1 c5_mv=4005 min_mv=4001 max_mv=4005 dV_mv=4 sum_mv=16011 pack_mv=16020 stack_mv=16011 i_pack_ma=320 i_cc2_ma=320 sample=1 alerts=2 i2c_err=0\r\n",now);
  snprintf(line,sizeof(line),"TC bq_ok=1 bq_vbat_mv=16020 bq_vsys_mv=16400 bq_ibat_ma=320 bq_ichg_ma=352 bq_idchg_ma=0 bq_vbus_mv=19980 bq_iin_ma=600 bq_vreg_mv=16800 bq_ichg_set_ma=1500 bq_iin_set_ma=3000 bq_st=0x8400 bq_fault=0x0 bq_in=1 bq_pre=0 bq_fast=1 bq_otg=%u bq_iindpm=0 bq_vindpm=0 tps_vbus_mv=20010 cc1=2 cc2=0 role=%u conn=1 plug=1 typec=0x42 rst=1 rst_busy=0 pd_role=%u pd_mv=20000 pd_ma=3000\r\n",role==2,role,role);
  receive(line,now);
}
#endif
