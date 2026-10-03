#include <images/BitmapDatabase.hpp>
#include <touchgfx/Bitmap.hpp>
#include <gui/screenextcharger_screen/ScreenExtChargerView.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/LabText.hpp>
void ScreenExtChargerView::setupScreen(){ScreenExtChargerViewBase::setupScreen();chart.setPosition(24,196,752,184);add(chart);selectField(1);}
void ScreenExtChargerView::selectField(unsigned i){field=i;PsuChgProfile& p=psu_charger()->profile;psu_editor_load_milli(&editor,i==0?p.cells*1000:i==1?p.cc_ma:i==2?p.term_ma:p.capacity_mah,3);if(i==0){snprintf(editor.text,sizeof(editor.text),"%u",p.cells);editor.length=strlen(editor.text);editor.replace_on_next=1;}refresh();}
void ScreenExtChargerView::chooseChem(unsigned i){if(psu_charger()->running)return;psu_chg_profile_defaults(&psu_charger()->profile,i);polarity=false;selectField(field);}
void ScreenExtChargerView::refresh(){
 PsuCharger* c=psu_charger();const PsuChgProfile& p=c->profile;char b[700];const bool setup=page==0;
 SetupButton.setBitmaps(touchgfx::Bitmap(page==0?BITMAP_UX_TAB_SEL_160X38_ID:BITMAP_UX_TAB_REL_160X38_ID),touchgfx::Bitmap(BITMAP_UX_TAB_SEL_160X38_ID));SetupButton.invalidate();
 SessionButton.setBitmaps(touchgfx::Bitmap(page==1?BITMAP_UX_TAB_SEL_160X38_ID:BITMAP_UX_TAB_REL_160X38_ID),touchgfx::Bitmap(BITMAP_UX_TAB_SEL_160X38_ID));SessionButton.invalidate();
 OnboardButton.setBitmaps(touchgfx::Bitmap(page==2?BITMAP_UX_TAB_SEL_160X38_ID:BITMAP_UX_TAB_REL_160X38_ID),touchgfx::Bitmap(BITMAP_UX_TAB_SEL_160X38_ID));OnboardButton.invalidate();
 Chem0.setBitmaps(touchgfx::Bitmap(p.chemistry==0?BITMAP_UX_ACTION_SEL_88X48_ID:BITMAP_UX_ACTION_REL_88X48_ID),touchgfx::Bitmap(BITMAP_UX_ACTION_SEL_88X48_ID));lab_enable(Chem0,!c->running);
 Chem1.setBitmaps(touchgfx::Bitmap(p.chemistry==1?BITMAP_UX_ACTION_SEL_88X48_ID:BITMAP_UX_ACTION_REL_88X48_ID),touchgfx::Bitmap(BITMAP_UX_ACTION_SEL_88X48_ID));lab_enable(Chem1,!c->running);
 Chem2.setBitmaps(touchgfx::Bitmap(p.chemistry==2?BITMAP_UX_ACTION_SEL_88X48_ID:BITMAP_UX_ACTION_REL_88X48_ID),touchgfx::Bitmap(BITMAP_UX_ACTION_SEL_88X48_ID));lab_enable(Chem2,!c->running);
 Chem3.setBitmaps(touchgfx::Bitmap(p.chemistry==3?BITMAP_UX_ACTION_SEL_88X48_ID:BITMAP_UX_ACTION_REL_88X48_ID),touchgfx::Bitmap(BITMAP_UX_ACTION_SEL_88X48_ID));lab_enable(Chem3,!c->running);
 Chem4.setBitmaps(touchgfx::Bitmap(p.chemistry==4?BITMAP_UX_ACTION_SEL_88X48_ID:BITMAP_UX_ACTION_REL_88X48_ID),touchgfx::Bitmap(BITMAP_UX_ACTION_SEL_88X48_ID));lab_enable(Chem4,!c->running);
 const char* names[]={"Li-ion","LiHV","LiFePO4","LTO","Lead"};
 snprintf(b,sizeof(b),"%s / %s / %s",page==2?"ONBOARD BQ25731":"EXTERNAL BATTERY / LDO",c->running?"RUNNING":"READY",c->reason);
 lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,b,lab_muted());
 chart.setVisible(page==1);chart.invalidate();SessionStats.setVisible(page==1);SessionStats.invalidate();SessionAxis.setVisible(page==1);SessionAxis.invalidate();OnboardRight.setVisible(page==2);OnboardRight.invalidate();OnboardData.setVisible(page==2);OnboardData.invalidate();
 EditHeading.setVisible(setup);EditHeading.invalidate();
 ProfileNote.setVisible(setup);ProfileNote.invalidate();
 PolarityButton.setVisible(setup);PolarityButton.invalidate();
 StartButton.setVisible(setup);StartButton.invalidate();
 StopButton.setVisible(setup);StopButton.invalidate();
 Chem0.setVisible(setup);Chem0.invalidate();
 Chem1.setVisible(setup);Chem1.invalidate();
 Chem2.setVisible(setup);Chem2.invalidate();
 Chem3.setVisible(setup);Chem3.invalidate();
 Chem4.setVisible(setup);Chem4.invalidate();
 EditCard0.setVisible(setup);EditCard0.invalidate();
 EditLabel0.setVisible(setup);EditLabel0.invalidate();
 EditValue0.setVisible(setup);EditValue0.invalidate();
 EditCard1.setVisible(setup);EditCard1.invalidate();
 EditLabel1.setVisible(setup);EditLabel1.invalidate();
 EditValue1.setVisible(setup);EditValue1.invalidate();
 EditCard2.setVisible(setup);EditCard2.invalidate();
 EditLabel2.setVisible(setup);EditLabel2.invalidate();
 EditValue2.setVisible(setup);EditValue2.invalidate();
 EditCard3.setVisible(setup);EditCard3.invalidate();
 EditLabel3.setVisible(setup);EditLabel3.invalidate();
 EditValue3.setVisible(setup);EditValue3.invalidate();
 ChargeKey0.setVisible(setup);ChargeKey0.invalidate();
 ChargeKey1.setVisible(setup);ChargeKey1.invalidate();
 ChargeKey2.setVisible(setup);ChargeKey2.invalidate();
 ChargeKey3.setVisible(setup);ChargeKey3.invalidate();
 ChargeKey4.setVisible(setup);ChargeKey4.invalidate();
 ChargeKey5.setVisible(setup);ChargeKey5.invalidate();
 ChargeKey6.setVisible(setup);ChargeKey6.invalidate();
 ChargeKey7.setVisible(setup);ChargeKey7.invalidate();
 ChargeKey8.setVisible(setup);ChargeKey8.invalidate();
 ChargeKey9.setVisible(setup);ChargeKey9.invalidate();
 ChargeKeyDot.setVisible(setup);ChargeKeyDot.invalidate();
 ChargeKeyClr.setVisible(setup);ChargeKeyClr.invalidate();
 ChargeKeyDel.setVisible(setup);ChargeKeyDel.invalidate();
 ChargeKeyApply.setVisible(setup);ChargeKeyApply.invalidate();
 lab_enable(ChargeKeyDot,field!=0&&!c->running);
 if(setup){
 const char* fields[]={"SERIES CELLS","CHARGE CURRENT / A","CUTOFF CURRENT / A","CAPACITY / Ah"};
 lab_show(EditHeading,EditHeadingBuffer,EDITHEADING_SIZE,fields[field],lab_cyan());
 {uint32_t v=p.cells*1000U;snprintf(b,sizeof(b),"%lu.%03lu",(unsigned long)(v/1000),(unsigned long)(v%1000));snprintf(b,sizeof(b),"%u S",p.cells);lab_show(EditValue0,EditValue0Buffer,EDITVALUE0_SIZE,field==0?editor.text:b,field==0?lab_cyan():lab_text());lab_enable(EditCard0,!c->running);}
 {uint32_t v=p.cc_ma;snprintf(b,sizeof(b),"%lu.%03lu",(unsigned long)(v/1000),(unsigned long)(v%1000));lab_show(EditValue1,EditValue1Buffer,EDITVALUE1_SIZE,field==1?editor.text:b,field==1?lab_cyan():lab_text());lab_enable(EditCard1,!c->running);}
 {uint32_t v=p.term_ma;snprintf(b,sizeof(b),"%lu.%03lu",(unsigned long)(v/1000),(unsigned long)(v%1000));lab_show(EditValue2,EditValue2Buffer,EDITVALUE2_SIZE,field==2?editor.text:b,field==2?lab_cyan():lab_text());lab_enable(EditCard2,!c->running);}
 {uint32_t v=p.capacity_mah;snprintf(b,sizeof(b),"%lu.%03lu",(unsigned long)(v/1000),(unsigned long)(v%1000));lab_show(EditValue3,EditValue3Buffer,EDITVALUE3_SIZE,field==3?editor.text:b,field==3?lab_cyan():lab_text());lab_enable(EditCard3,!c->running);}
 snprintf(b,sizeof(b),"%s / CV %lu.%03lu V / max %lu mA (0.5C)\nTemperature monitoring unavailable over UART",names[p.chemistry<5?p.chemistry:0],(unsigned long)(p.cells*p.cv_mv_cell/1000),(unsigned long)(p.cells*p.cv_mv_cell%1000),(unsigned long)(p.capacity_mah/2));
 lab_show(ProfileNote,ProfileNoteBuffer,PROFILENOTE_SIZE,b,lab_muted());lab_enable(StartButton,!c->running&&polarity);lab_enable(PolarityButton,!c->running);lab_enable(StopButton,c->running);
 }
 if(page==1){snprintf(b,sizeof(b),"%02lu:%02lu:%02lu    %lu mAh    %lu.%03lu Wh\n%s",(unsigned long)(c->elapsed_ms/3600000),(unsigned long)(c->elapsed_ms/60000%60),(unsigned long)(c->elapsed_ms/1000%60),(unsigned long)c->delivered_mah,(unsigned long)(c->delivered_mwh/1000),(unsigned long)(c->delivered_mwh%1000),c->trace_count?psu_chg_state_name(c->state):"Start charging to record a session");lab_show(SessionStats,SessionStatsBuffer,SESSIONSTATS_SIZE,b,lab_text());
 uint32_t mv=p.cells*p.cv_mv_cell*11/10,ma=p.cc_ma*12/10;for(unsigned i=0;i<c->trace_count;i++){if(c->trace_mv[i]>mv)mv=c->trace_mv[i];if(c->trace_ma[i]>ma)ma=c->trace_ma[i];}
 snprintf(b,sizeof(b),"0 s   |   BLUE: 0-%lu.%02lu V    GREEN: 0-%lu.%02lu A   |   %lu s",(unsigned long)(mv/1000),(unsigned long)(mv%1000/10),(unsigned long)(ma/1000),(unsigned long)(ma%1000/10),(unsigned long)(c->elapsed_ms/1000));lab_show(SessionAxis,SessionAxisBuffer,SESSIONAXIS_SIZE,b,lab_muted());}
 if(page==2){TelemetryData d;telemetry_page(PAGE_CHARGER,d);snprintf(b,sizeof(b),"VBAT %s / IBAT %s\nVSYS %s / IIN %s\n\n%s",d.metric[0],d.metric[1],d.metric[2],d.metric[3],d.left);lab_show(OnboardData,OnboardDataBuffer,ONBOARDDATA_SIZE,b,lab_text());lab_show(OnboardRight,OnboardRightBuffer,ONBOARDRIGHT_SIZE,d.right,lab_text());}
}
void ScreenExtChargerView::key(char k){if(psu_charger()->running)return;psu_editor_key(&editor,k);refresh();}
void ScreenExtChargerView::chargeKeyClr(){if(psu_charger()->running)return;psu_editor_clear(&editor);refresh();}
void ScreenExtChargerView::chargeKeyDel(){if(psu_charger()->running)return;psu_editor_backspace(&editor);refresh();}
void ScreenExtChargerView::chargeKeyApply(){PsuCharger* c=psu_charger();if(c->running)return;uint32_t v;if(!psu_editor_parse_milli(&editor,&v)){psu_editor_clear(&editor);refresh();return;}PsuChgProfile& p=c->profile;
 if(field==0){v/=1000;if(v<1)v=1;if(v>psu_chg_max_cells(&p))v=psu_chg_max_cells(&p);p.cells=v;}
 if(field==1){uint32_t max=p.capacity_mah/2;if(max>5000)max=5000;if(v>max)v=max;if(v<1)v=1;p.cc_ma=v;if(p.term_ma>v)p.term_ma=v;}
 if(field==2){if(v>p.cc_ma)v=p.cc_ma;if(v<1)v=1;p.term_ma=v;}
 if(field==3){if(v>100000)v=100000;if(v<2)v=2;p.capacity_mah=v;if(p.cc_ma>v/2)p.cc_ma=v/2;if(p.term_ma>p.cc_ma)p.term_ma=p.cc_ma;}
 if(p.precharge_ma>p.cc_ma)p.precharge_ma=p.cc_ma;
 polarity=false;selectField(field);
}
void ScreenExtChargerView::showSetup(){page=0;refresh();}
void ScreenExtChargerView::showSession(){page=1;refresh();}
void ScreenExtChargerView::showOnboard(){page=2;refresh();}
void ScreenExtChargerView::confirmPolarity(){polarity=true;snprintf(psu_charger()->reason,64,"POLARITY CONFIRMED");refresh();}
void ScreenExtChargerView::startCharge(){PsuSnapshot live;psu_snapshot(&live);PsuCharger* c=psu_charger();
 if(!polarity||psu_seq_edit_locked(psu_sequencer())||live.shutdown_pending){snprintf(c->reason,64,"Stop other automation first");refresh();return;}
 PsuChgSense sense={};sense.pack_mv=live.vout_mv;sense.current_ma=live.display_current_ua/1000;sense.temp_centi=live.mos_centi;sense.telemetry_ok=live.g0_connected&&!live.g0_stale&&live.current_valid&&(!c->profile.temp_sensor||live.temperature_valid);sense.permit=!live.fault_latched&&psu_g4()->telemetry.permit;
 c->profile.confirmed=1;c->profile.polarity_checked=1;
 if(psu_chg_start(c,&c->profile,&sense,psu_app_now())){page=1;}
 refresh();}
void ScreenExtChargerView::stopCharge(){psu_chg_abort(psu_charger(),"Operator stop",psu_app_now());polarity=false;refresh();}
void ScreenExtChargerView::allOff(){psu_app_shutdown();polarity=false;refresh();}
void ScreenExtChargerView::chem0(){chooseChem(0);}
void ScreenExtChargerView::chem1(){chooseChem(1);}
void ScreenExtChargerView::chem2(){chooseChem(2);}
void ScreenExtChargerView::chem3(){chooseChem(3);}
void ScreenExtChargerView::chem4(){chooseChem(4);}
void ScreenExtChargerView::field0(){selectField(0);}
void ScreenExtChargerView::field1(){selectField(1);}
void ScreenExtChargerView::field2(){selectField(2);}
void ScreenExtChargerView::field3(){selectField(3);}
void ScreenExtChargerView::chargeKey0(){key('0');}
void ScreenExtChargerView::chargeKey1(){key('1');}
void ScreenExtChargerView::chargeKey2(){key('2');}
void ScreenExtChargerView::chargeKey3(){key('3');}
void ScreenExtChargerView::chargeKey4(){key('4');}
void ScreenExtChargerView::chargeKey5(){key('5');}
void ScreenExtChargerView::chargeKey6(){key('6');}
void ScreenExtChargerView::chargeKey7(){key('7');}
void ScreenExtChargerView::chargeKey8(){key('8');}
void ScreenExtChargerView::chargeKey9(){key('9');}
void ScreenExtChargerView::chargeKeyDot(){key('.');}
