#include <gui/common/UiTheme.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Bitmap.hpp>
#include <stdlib.h>
#include <gui/screenextcharger_screen/ScreenExtChargerView.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/FrontendApplication.hpp>
namespace {
void visible(touchgfx::Drawable& w,bool show){if(w.isVisible()!=show){w.invalidate();w.setVisible(show);w.invalidate();}}
}
void ScreenExtChargerView::setupScreen(){
 ScreenExtChargerViewBase::setupScreen();chart.setPosition(524,282,248,72);add(chart);
 selectField(1);editing=false;refresh();setupTheme();
}
void ScreenExtChargerView::handleTickEvent(){
 if(static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->isScreenTransitionActive())return;
 if(++divider>=6){divider=0;refresh();}
}
void ScreenExtChargerView::selectField(unsigned i){
 if(psu_charger()->running)return;
 if(editing&&field==i){editing=false;refresh();return;}
 field=i;editing=true;const PsuChgProfile& p=psu_charger()->profile;
 psu_editor_load_milli(&editor,i==0?p.cells*1000:i==1?p.cc_ma:i==2?p.term_ma:psu_chg_target_mv(&p),3);
 if(i==0){snprintf(editor.text,sizeof(editor.text),"%u",p.cells);editor.length=strlen(editor.text);editor.replace_on_next=1;}
 refresh();
}
void ScreenExtChargerView::chooseChem(unsigned i){
 if(psu_charger()->running)return;
 psu_chg_profile_defaults(&psu_charger()->profile,i);polarity=false;editing=false;refresh();
}
void ScreenExtChargerView::refresh(){
 PsuCharger *c=psu_charger();const PsuChgProfile& p=c->profile;PsuSnapshot live;psu_snapshot(&live);char b[300];
 if(c->running)editing=false;
 snprintf(b,sizeof(b),"OUTPUT CHARGER / %s / %s",psu_chg_state_name(c->state),c->reason);
 lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,b,c->state==CHG_FAULT||c->state==CHG_ABORTED?lab_red():lab_muted());
 touchgfx::ButtonWithLabel* chemistry[]={&Chem0,&Chem1,&Chem2,&Chem3,&Chem4};
 for(unsigned i=0;i<5;i++){
  const auto id=p.chemistry==i?BITMAP_UX_ACTION_SEL_88X48_ID:BITMAP_UX_ACTION_REL_88X48_ID;
  if(chemistry[i]->getCurrentlyDisplayedBitmap().getId()!=id){chemistry[i]->setBitmaps(touchgfx::Bitmap(id),touchgfx::Bitmap(BITMAP_UX_ACTION_SEL_88X48_ID));chemistry[i]->invalidate();}
  lab_enable(*chemistry[i],!c->running);
 }
 touchgfx::Button* fields[]={&EditCard0,&EditCard1,&EditCard2,&EditCard3};
 for(auto* f:fields)lab_enable(*f,!c->running);
 const char* labels[]={"SERIES CELLS","CHARGE CURRENT / A","STOP BELOW CURRENT / A","","TARGET PACK VOLTAGE / V"};
 visible(EditHeading,editing);lab_show(EditHeading,EditHeadingBuffer,EDITHEADING_SIZE,labels[field],lab_cyan());
 snprintf(b,sizeof(b),"%u S",p.cells);lab_show(EditValue0,EditValue0Buffer,EDITVALUE0_SIZE,editing&&field==0?editor.text:b,editing&&field==0?lab_cyan():lab_text());
 snprintf(b,sizeof(b),"%lu.%03lu",(unsigned long)(p.cc_ma/1000),(unsigned long)(p.cc_ma%1000));lab_show(EditValue1,EditValue1Buffer,EDITVALUE1_SIZE,editing&&field==1?editor.text:b,editing&&field==1?lab_cyan():lab_text());
 snprintf(b,sizeof(b),"%lu.%03lu",(unsigned long)(p.term_ma/1000),(unsigned long)(p.term_ma%1000));lab_show(EditValue2,EditValue2Buffer,EDITVALUE2_SIZE,editing&&field==2?editor.text:b,editing&&field==2?lab_cyan():lab_text());
 const uint32_t target=psu_chg_target_mv(&p);
 snprintf(b,sizeof(b),"%lu.%03lu",(unsigned long)(target/1000),(unsigned long)(target%1000));lab_show(EditValue3,EditValue3Buffer,EDITVALUE3_SIZE,editing&&field==4?editor.text:b,editing&&field==4?lab_cyan():lab_text());
 const char* names[]={"Li-ion","LiHV","LiFePO4","LTO","Lead"};
 snprintf(b,sizeof(b),"%s / output terminals\n%s\nStop current applies after reaching the target voltage.",names[p.chemistry<5?p.chemistry:0],editing?"APPLY to save / tap the selected field to cancel.":"Tap a field to edit. Confirm polarity before START.");
 lab_show(ProfileNote,ProfileNoteBuffer,PROFILENOTE_SIZE,b,lab_muted());
 lab_enable(StartButton,!c->running&&polarity&&!editing);lab_enable(PolarityButton,!c->running&&!editing);
 lab_enable(StopButton,c->running||live.output_requested||live.output_confirmed||live.shutdown_pending);
 touchgfx::Drawable* keys[]={&ChargeKey0,&ChargeKey1,&ChargeKey2,&ChargeKey3,&ChargeKey4,&ChargeKey5,&ChargeKey6,&ChargeKey7,&ChargeKey8,&ChargeKey9,&ChargeKeyDot,&ChargeKeyClr,&ChargeKeyDel,&ChargeKeyApply};
 for(auto* key:keys)visible(*key,editing);
 lab_enable(ChargeKeyDot,field!=0&&!c->running);
 visible(SessionCard,!editing);visible(SessionHeading,!editing);visible(SessionStats,!editing);visible(SessionAxis,!editing);
 visible(LiveVoltage,!editing);visible(LiveCurrent,!editing);visible(chart,!editing);visible(OnboardButton,!editing);
 if(!editing){
  if(live.g0_connected&&!live.g0_stale)snprintf(b,sizeof(b),"V %lu.%03lu V",(unsigned long)(live.vout_mv/1000),(unsigned long)(live.vout_mv%1000));else strcpy(b,"V -- V");
  lab_show(LiveVoltage,LiveVoltageBuffer,LIVEVOLTAGE_SIZE,b,SessionChart::voltageColor());
  if(live.current_valid)snprintf(b,sizeof(b),"I %s%lu.%03lu A",live.display_current_ua<0?"-":"",(unsigned long)(llabs(live.display_current_ua)/1000000),(unsigned long)(llabs(live.display_current_ua)/1000%1000));else strcpy(b,"I -- A");
  lab_show(LiveCurrent,LiveCurrentBuffer,LIVECURRENT_SIZE,b,SessionChart::currentColor());
  snprintf(b,sizeof(b),"%s\nTime   %02lu:%02lu:%02lu\nCharge %lu mAh\nEnergy %lu.%03lu Wh",psu_chg_state_name(c->state),(unsigned long)(c->elapsed_ms/3600000),(unsigned long)(c->elapsed_ms/60000%60),(unsigned long)(c->elapsed_ms/1000%60),(unsigned long)c->delivered_mah,(unsigned long)(c->delivered_mwh/1000),(unsigned long)(c->delivered_mwh%1000));
  lab_show(SessionStats,SessionStatsBuffer,SESSIONSTATS_SIZE,b,lab_text());chart.update(c);
  snprintf(b,sizeof(b),"V %lu.%02lu-%lu.%02lu V\nI %lu.%02lu-%lu.%02lu A / %lu:%02lu",(unsigned long)(chart.voltageMin/1000),(unsigned long)(chart.voltageMin%1000/10),(unsigned long)(chart.voltageMax/1000),(unsigned long)(chart.voltageMax%1000/10),(unsigned long)(chart.currentMin/1000),(unsigned long)(chart.currentMin%1000/10),(unsigned long)(chart.currentMax/1000),(unsigned long)(chart.currentMax%1000/10),(unsigned long)(chart.timeMax/60000),(unsigned long)(chart.timeMax/1000%60));
  lab_show(SessionAxis,SessionAxisBuffer,SESSIONAXIS_SIZE,b,lab_muted());
 }
 ui::ThemeScreen::get().sync();
}
void ScreenExtChargerView::key(char k){if(psu_charger()->running)return;psu_editor_key(&editor,k);refresh();}
void ScreenExtChargerView::chargeKeyClr(){if(psu_charger()->running)return;psu_editor_clear(&editor);refresh();}
void ScreenExtChargerView::chargeKeyDel(){if(psu_charger()->running)return;psu_editor_backspace(&editor);refresh();}
void ScreenExtChargerView::chargeKeyApply(){PsuCharger* c=psu_charger();if(c->running)return;uint32_t v;if(!psu_editor_parse_milli(&editor,&v)){psu_editor_clear(&editor);refresh();return;}PsuChgProfile& p=c->profile;
 if(field==0){v/=1000;if(v<1)v=1;if(v>psu_chg_max_cells(&p))v=psu_chg_max_cells(&p);p.cells=v;p.target_pack_mv=0;}
 if(field==1){uint32_t max=5000;if(v>max)v=max;if(v<1)v=1;p.cc_ma=v;if(p.term_ma>v)p.term_ma=v;}
 if(field==2){if(v>p.cc_ma)v=p.cc_ma;if(v<1)v=1;p.term_ma=v;}
 if(field==4 && !psu_chg_set_target(&p,v)){snprintf(c->reason,64,"Target must be %lu-%lu mV",(unsigned long)(p.cells*p.precharge_mv_cell),(unsigned long)(p.cells*p.cv_mv_cell));refresh();return;}
 if(p.precharge_ma>p.cc_ma)p.precharge_ma=p.cc_ma;
 polarity=false;editing=true;selectField(field);
}
void ScreenExtChargerView::showSetup(){editing=false;refresh();}
void ScreenExtChargerView::showSession(){editing=false;refresh();}
void ScreenExtChargerView::showOnboard(){static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->gotoScreenBatteryScreenWipeTransitionWest();}
void ScreenExtChargerView::confirmPolarity(){polarity=true;snprintf(psu_charger()->reason,64,"POLARITY CONFIRMED");refresh();}
void ScreenExtChargerView::startCharge(){PsuSnapshot live;psu_snapshot(&live);PsuCharger* c=psu_charger();
 if(editing||!polarity||psu_seq_edit_locked(psu_sequencer())||live.shutdown_pending){snprintf(c->reason,64,"Stop other automation first");refresh();return;}
 c->profile.confirmed=1;c->profile.polarity_checked=1;
 if(psu_app_start_charging()){editing=false;}
 refresh();}
void ScreenExtChargerView::stopCharge(){psu_app_shutdown();snprintf(psu_charger()->reason,sizeof(psu_charger()->reason),"Operator stop");polarity=false;refresh();}
void ScreenExtChargerView::allOff(){psu_app_power_shutdown();polarity=false;refresh();}
void ScreenExtChargerView::chem0(){chooseChem(0);}
void ScreenExtChargerView::chem1(){chooseChem(1);}
void ScreenExtChargerView::chem2(){chooseChem(2);}
void ScreenExtChargerView::chem3(){chooseChem(3);}
void ScreenExtChargerView::chem4(){chooseChem(4);}
void ScreenExtChargerView::field0(){selectField(0);}
void ScreenExtChargerView::field1(){selectField(1);}
void ScreenExtChargerView::field2(){selectField(2);}
void ScreenExtChargerView::field3(){selectField(4);}
void ScreenExtChargerView::field4(){selectField(4);}
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

void ScreenExtChargerView::setupTheme()
{
    ui::ThemeScreen& theme=ui::ThemeScreen::get();
    theme.begin(*this);
    theme.box(LabBackground,ui::BACKGROUND);
    theme.box(LabHeader,ui::SURFACE);
    theme.box(ThemeHeaderDivider,ui::BORDER);
    theme.text(PageTitle);
    theme.text(PageFeedback);
    theme.button(AllOffButton,ui::DANGER);
    theme.button(BackButton,ui::NORMAL);
    theme.button(ChargeKey1,ui::NORMAL);
    theme.button(ChargeKey2,ui::NORMAL);
    theme.button(ChargeKey3,ui::NORMAL);
    theme.button(ChargeKey4,ui::NORMAL);
    theme.button(ChargeKey5,ui::NORMAL);
    theme.button(ChargeKey6,ui::NORMAL);
    theme.button(ChargeKey7,ui::NORMAL);
    theme.button(ChargeKey8,ui::NORMAL);
    theme.button(ChargeKey9,ui::NORMAL);
    theme.button(ChargeKeyClr,ui::NORMAL);
    theme.button(ChargeKey0,ui::NORMAL);
    theme.button(ChargeKeyDot,ui::NORMAL);
    theme.button(ChargeKeyDel,ui::NORMAL);
    theme.button(ChargeKeyApply,ui::PRIMARY);


    theme.button(OnboardButton,ui::NORMAL);
    theme.text(EditHeading);
    theme.button(Chem0,ui::NORMAL);
    theme.button(Chem1,ui::NORMAL);
    theme.button(Chem2,ui::NORMAL);
    theme.button(Chem3,ui::NORMAL);
    theme.button(Chem4,ui::NORMAL);
    theme.button(EditCard0,ui::NORMAL);
    theme.text(EditLabel0);
    theme.text(EditValue0);
    theme.button(EditCard1,ui::NORMAL);
    theme.text(EditLabel1);
    theme.text(EditValue1);
    theme.button(EditCard2,ui::NORMAL);
    theme.text(EditLabel2);
    theme.text(EditValue2);
    theme.button(EditCard3,ui::NORMAL);
    theme.text(EditLabel3);
    theme.text(EditValue3);
    theme.text(ProfileNote);
    theme.button(PolarityButton,ui::NORMAL);
    theme.button(StartButton,ui::NORMAL);
    theme.button(StopButton,ui::DANGER);
    theme.panel(SessionCard);theme.text(SessionHeading,ui::ACCENT);
    theme.text(SessionStats);
    theme.text(LiveVoltage);theme.text(LiveCurrent);









    theme.text(SessionAxis);


    theme.apply();
}
