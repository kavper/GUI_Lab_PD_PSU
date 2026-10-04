#include <gui/common/UiTheme.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Bitmap.hpp>
#include <gui/screendiagnostics_screen/ScreenDiagnosticsView.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/LabText.hpp>
static void formatValue(char* out,size_t n,const char* key,int64_t val){
 const char* suffix=strrchr(key,'_');const char* unit=0;
 if(suffix&&!strcmp(suffix,"_mv"))unit="V";
 if(suffix&&!strcmp(suffix,"_ma"))unit="A";
 if(suffix&&!strcmp(suffix,"_mw"))unit="W";
 if(unit){uint64_t a=val<0?-val:val;snprintf(out,n,"%s%llu.%03llu %s",val<0?"-":"",(unsigned long long)(a/1000),(unsigned long long)(a%1000),unit);}
 else if(suffix&&!strcmp(suffix,"_ms"))snprintf(out,n,"%lld ms",(long long)val);
 else if(suffix&&!strcmp(suffix,"_x10"))snprintf(out,n,"%lld.%lld %%",(long long)(val/10),(long long)(val%10));
 else if(!strcmp(key,"mode"))snprintf(out,n,"%s",val==2?"CC":val==1?"CV":"IDLE");
 else if(!strcmp(key,"pd_role"))snprintf(out,n,"%s",val==1?"SINK":val==2?"SOURCE":"NONE");
 else if(strstr(key,"fault")||!strcmp(key,"sa")||!strcmp(key,"sb")||!strcmp(key,"sc")||!strcmp(key,"alarm"))snprintf(out,n,"0x%lX",(unsigned long)val);
 else snprintf(out,n,"%lld",(long long)val);
}
void ScreenDiagnosticsView::refresh(){
 ParsedButton.setBitmaps(touchgfx::Bitmap(!raw?BITMAP_BTN_BACK_PRS_112X48_ID:BITMAP_BTN_BACK_REL_112X48_ID),touchgfx::Bitmap(BITMAP_BTN_BACK_PRS_112X48_ID));ParsedButton.invalidate();
 RawButton.setBitmaps(touchgfx::Bitmap(raw?BITMAP_BTN_BACK_PRS_112X48_ID:BITMAP_BTN_BACK_REL_112X48_ID),touchgfx::Bitmap(BITMAP_BTN_BACK_PRS_112X48_ID));RawButton.invalidate();
 TButton.setBitmaps(touchgfx::Bitmap(kind==0?BITMAP_BTN_BACK_PRS_112X48_ID:BITMAP_BTN_BACK_REL_112X48_ID),touchgfx::Bitmap(BITMAP_BTN_BACK_PRS_112X48_ID));TButton.invalidate();
 TBButton.setBitmaps(touchgfx::Bitmap(kind==1?BITMAP_BTN_BACK_PRS_112X48_ID:BITMAP_BTN_BACK_REL_112X48_ID),touchgfx::Bitmap(BITMAP_BTN_BACK_PRS_112X48_ID));TBButton.invalidate();
 TCButton.setBitmaps(touchgfx::Bitmap(kind==2?BITMAP_BTN_BACK_PRS_112X48_ID:BITMAP_BTN_BACK_REL_112X48_ID),touchgfx::Bitmap(BITMAP_BTN_BACK_PRS_112X48_ID));TCButton.invalidate();
 TelemetryRecord rec(kind);G4Port* host=psu_g4();char b[1600];
 snprintf(b,sizeof(b),"%s / %s / RX %lu / parse errors %lu / ACK %lu / ERR %lu",kind==0?"METER":kind==1?"BMS":"PD",rec.fresh?"LIVE":"STALE",(unsigned long)host->rx_lines,(unsigned long)host->parse_error_count,(unsigned long)host->ack_count,(unsigned long)host->err_count);
 lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,b,rec.fresh?lab_muted():lab_amber());
 unsigned count=0;while(g4_record_key(kind,count))++count;
 char source[G4_RX_LINE_MAX];g4_raw_snapshot(host,kind,source,sizeof(source));
 const unsigned columns=76;unsigned rawLines=(strlen(source)+columns-1)/columns;
 unsigned maxOffset=raw?(rawLines>9?rawLines-9:0):(count>14?(count-14+1)/2:0);
 if(offset>maxOffset)offset=maxOffset;
 RawText.setVisible(raw);RawText.invalidate();
 if(raw){unsigned dst=0;for(unsigned i=offset*columns;source[i]&&dst<sizeof(b)-2;i++){if(i>offset*columns&&(i-offset*columns)%columns==0)b[dst++]='\n';if((i-offset*columns)/columns>=9)break;b[dst++]=source[i];}b[dst]=0;lab_show(RawText,RawTextBuffer,RAWTEXT_SIZE,source[0]?b:"Waiting for a complete UART frame",lab_text());}
 FieldBox0.setVisible(!raw);FieldBox0.invalidate();Field0.setVisible(!raw);Field0.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+0);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field0,Field0Buffer,FIELD0_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox1.setVisible(!raw);FieldBox1.invalidate();Field1.setVisible(!raw);Field1.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+1);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field1,Field1Buffer,FIELD1_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox2.setVisible(!raw);FieldBox2.invalidate();Field2.setVisible(!raw);Field2.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+2);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field2,Field2Buffer,FIELD2_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox3.setVisible(!raw);FieldBox3.invalidate();Field3.setVisible(!raw);Field3.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+3);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field3,Field3Buffer,FIELD3_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox4.setVisible(!raw);FieldBox4.invalidate();Field4.setVisible(!raw);Field4.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+4);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field4,Field4Buffer,FIELD4_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox5.setVisible(!raw);FieldBox5.invalidate();Field5.setVisible(!raw);Field5.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+5);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field5,Field5Buffer,FIELD5_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox6.setVisible(!raw);FieldBox6.invalidate();Field6.setVisible(!raw);Field6.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+6);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field6,Field6Buffer,FIELD6_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox7.setVisible(!raw);FieldBox7.invalidate();Field7.setVisible(!raw);Field7.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+7);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field7,Field7Buffer,FIELD7_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox8.setVisible(!raw);FieldBox8.invalidate();Field8.setVisible(!raw);Field8.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+8);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field8,Field8Buffer,FIELD8_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox9.setVisible(!raw);FieldBox9.invalidate();Field9.setVisible(!raw);Field9.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+9);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field9,Field9Buffer,FIELD9_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox10.setVisible(!raw);FieldBox10.invalidate();Field10.setVisible(!raw);Field10.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+10);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field10,Field10Buffer,FIELD10_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox11.setVisible(!raw);FieldBox11.invalidate();Field11.setVisible(!raw);Field11.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+11);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field11,Field11Buffer,FIELD11_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox12.setVisible(!raw);FieldBox12.invalidate();Field12.setVisible(!raw);Field12.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+12);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field12,Field12Buffer,FIELD12_SIZE,b,rec.fresh?lab_text():lab_muted());}
 FieldBox13.setVisible(!raw);FieldBox13.invalidate();Field13.setVisible(!raw);Field13.invalidate();
 if(!raw){const char* key=g4_record_key(kind,offset*2+13);int64_t val;if(key&&rec.get(key,val)){char fv[40];formatValue(fv,sizeof(fv),key,val);snprintf(b,sizeof(b),"%-18s %s",key,fv);}else snprintf(b,sizeof(b),"%s%s",key?key:"",key?"  --":"");lab_show(Field13,Field13Buffer,FIELD13_SIZE,b,rec.fresh?lab_text():lab_muted());}

 lab_enable(UpButton,offset>0);lab_enable(DownButton,offset<maxOffset);
 PsuSnapshot live;psu_snapshot(&live);
 const char* phase=live.output_phase==G4_OUTPUT_STARTING?"START":live.output_phase==G4_OUTPUT_RUNNING?"RUN":live.output_phase==G4_OUTPUT_STOPPING?"STOP":"OFF";
 snprintf(b,sizeof(b),"H7 ON [%s%s]: %s\nEvent: %s\nG4 fault=0x%lX ctrl=%u latch=%u | G0 fault=0x%lX kill=%u",
     phase,live.fault_latched?" / LATCH":"",live.on_block_reason,
     live.fault_latched?live.fault_context:(live.last_on_reject[0]?live.last_on_reject:"none"),
     (unsigned long)host->telemetry.fault,host->telemetry.ctrl,host->telemetry.fault_latch,
     (unsigned long)host->telemetry.g0_fault,host->telemetry.kill);
 size_t used=strlen(b);
 if(host->nack_valid){
   const char* reasons[]={"UNKNOWN CODE","UNKNOWN","BAD_PAYLOAD","RANGE","UNSAFE","BUSY","TIMEOUT","LINK"};
   const char* reason=host->nack_reason<8?reasons[host->nack_reason]:reasons[0];
   snprintf(b+used,sizeof(b)-used,"\nLast NACK: TYPE=0x%02X SEQ=%u reason=%u %s (%s)",host->nack_type,
       host->nack_seq,host->nack_reason,reason,host->nack_matched?"matched":"unmatched");
 }else snprintf(b+used,sizeof(b)-used,"\nLast NACK: none");
 lab_show(FaultText,FaultTextBuffer,FAULTTEXT_SIZE,b,live.fault_latched?lab_red():strcmp(live.on_block_reason,"READY")?lab_amber():lab_muted());
 lab_enable(ClearButton,live.fault_latched&&host->telemetry.valid&&psu_app_now()-host->telemetry.ms<=50U);

}
void ScreenDiagnosticsView::allOff(){psu_app_shutdown();}
void ScreenDiagnosticsView::showParsed(){raw=false;offset=0;refresh();}
void ScreenDiagnosticsView::showRaw(){raw=true;offset=0;refresh();}
void ScreenDiagnosticsView::frameT(){frame(0);}
void ScreenDiagnosticsView::frameTB(){frame(1);}
void ScreenDiagnosticsView::frameTC(){frame(2);}
void ScreenDiagnosticsView::scrollUp(){if(offset)--offset;refresh();}
void ScreenDiagnosticsView::scrollDown(){++offset;refresh();}
void ScreenDiagnosticsView::clearFault(){psu_app_clear_fault();refresh();}
void ScreenDiagnosticsView::handleDragEvent(const touchgfx::DragEvent& e){
 if(e.getOldY()>=144&&e.getOldY()<392){drag+=e.getDeltaY();while(drag<=-28){scrollDown();drag+=28;}while(drag>=28){scrollUp();drag-=28;}}
 else ScreenDiagnosticsViewBase::handleDragEvent(e);
}

void ScreenDiagnosticsView::setupTheme()
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
    theme.button(ParsedButton,ui::NORMAL);
    theme.button(RawButton,ui::NORMAL);
    theme.button(SenseDetailsButton,ui::NORMAL);
    theme.button(TButton,ui::NORMAL);
    theme.button(TBButton,ui::NORMAL);
    theme.button(TCButton,ui::NORMAL);
    theme.panel(FieldBox0);
    theme.text(Field0);
    theme.panel(FieldBox1);
    theme.text(Field1);
    theme.panel(FieldBox2);
    theme.text(Field2);
    theme.panel(FieldBox3);
    theme.text(Field3);
    theme.panel(FieldBox4);
    theme.text(Field4);
    theme.panel(FieldBox5);
    theme.text(Field5);
    theme.panel(FieldBox6);
    theme.text(Field6);
    theme.panel(FieldBox7);
    theme.text(Field7);
    theme.panel(FieldBox8);
    theme.text(Field8);
    theme.panel(FieldBox9);
    theme.text(Field9);
    theme.panel(FieldBox10);
    theme.text(Field10);
    theme.panel(FieldBox11);
    theme.text(Field11);
    theme.panel(FieldBox12);
    theme.text(Field12);
    theme.panel(FieldBox13);
    theme.text(Field13);
    theme.text(RawText);
    theme.button(UpButton,ui::NORMAL);
    theme.button(DownButton,ui::NORMAL);
    theme.text(FaultText);
    theme.button(ClearButton,ui::NORMAL);
    theme.apply();
}
