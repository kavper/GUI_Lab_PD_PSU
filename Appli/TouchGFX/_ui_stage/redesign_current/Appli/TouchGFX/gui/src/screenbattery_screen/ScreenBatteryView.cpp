#include <gui/common/UiTheme.hpp>
#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/TelemetryData.hpp>
ScreenBatteryView::ScreenBatteryView() : divider(0), noticeTicks(0) {notice[0]=0;}
void ScreenBatteryView::setupScreen() {ScreenBatteryViewBase::setupScreen(); refresh();
    setupTheme();
}
void ScreenBatteryView::tearDownScreen() {ScreenBatteryViewBase::tearDownScreen();}
void ScreenBatteryView::handleTickEvent() {if(noticeTicks)--noticeTicks;if(++divider>=8){divider=0;refresh();}}
void ScreenBatteryView::notify(const char* text) {snprintf(notice,sizeof(notice),"%s",text);noticeTicks=180;refresh();}
void ScreenBatteryView::allOff() {psu_app_shutdown();notify("PSU OFF requested - stopping LDO, DCDC and automation");}
void ScreenBatteryView::refresh() {
    TelemetryData data;
    telemetry_page(PAGE_BATTERY,data);
    lab_show(Metric0,Metric0Buffer,METRIC0_SIZE,data.metric[0],data.fresh?lab_text():lab_muted());
    lab_show(Metric1,Metric1Buffer,METRIC1_SIZE,data.metric[1],data.fresh?lab_text():lab_muted());
    lab_show(Metric2,Metric2Buffer,METRIC2_SIZE,data.metric[2],data.fresh?lab_text():lab_muted());
    lab_show(Metric3,Metric3Buffer,METRIC3_SIZE,data.metric[3],data.fresh?lab_text():lab_muted());
    TelemetryRecord pack(G4_RECORD_TB);
    pack.fresh=pack.fresh&&pack.is("sample",1)&&pack.is("bms",1);
    char left[512]={0},right[512]={0};
    pack.row(left,sizeof(left),"Cell sum","sum_mv"," V",1000,3);pack.row(left,sizeof(left),"Stack voltage","stack_mv"," V",1000,3);
    pack.flag(right,sizeof(right),"Charge FET","chg");pack.flag(right,sizeof(right),"Discharge FET","dsg");
    lab_show(LeftDetails,LeftDetailsBuffer,LEFTDETAILS_SIZE,left,lab_text());lab_show(RightDetails,RightDetailsBuffer,RIGHTDETAILS_SIZE,right,lab_text());
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,noticeTicks?notice:data.status,data.fresh?lab_muted():lab_amber());
    char value[32];int64_t cell=0;
    pack.value(value,sizeof(value),"c1_mv"," V",1000,3);if(pack.get("c1_mv",cell)&&cell==-1)strcpy(value,"UNUSED");
    lab_show(CellValue0,CellValue0Buffer,CELLVALUE0_SIZE,value,lab_cyan());
    CellBar0.setColor(lab_cyan());CellBar0.setVisible(pack.get("c1_mv",cell)&&cell>=0);CellBar0.setWidth(cell>=0?static_cast<int16_t>((cell>5000?5000:cell)*120/5000):0);CellBar0.invalidate();
    pack.value(value,sizeof(value),"c2_mv"," V",1000,3);if(pack.get("c2_mv",cell)&&cell==-1)strcpy(value,"UNUSED");
    lab_show(CellValue1,CellValue1Buffer,CELLVALUE1_SIZE,value,lab_cyan());
    CellBar1.setColor(lab_cyan());CellBar1.setVisible(pack.get("c2_mv",cell)&&cell>=0);CellBar1.setWidth(cell>=0?static_cast<int16_t>((cell>5000?5000:cell)*120/5000):0);CellBar1.invalidate();
    pack.value(value,sizeof(value),"c3_mv"," V",1000,3);if(pack.get("c3_mv",cell)&&cell==-1)strcpy(value,"UNUSED");
    lab_show(CellValue2,CellValue2Buffer,CELLVALUE2_SIZE,value,lab_cyan());
    CellBar2.setColor(lab_cyan());CellBar2.setVisible(pack.get("c3_mv",cell)&&cell>=0);CellBar2.setWidth(cell>=0?static_cast<int16_t>((cell>5000?5000:cell)*120/5000):0);CellBar2.invalidate();
    pack.value(value,sizeof(value),"c4_mv"," V",1000,3);if(pack.get("c4_mv",cell)&&cell==-1)strcpy(value,"UNUSED");
    lab_show(CellValue3,CellValue3Buffer,CELLVALUE3_SIZE,value,lab_cyan());
    CellBar3.setColor(lab_cyan());CellBar3.setVisible(pack.get("c4_mv",cell)&&cell>=0);CellBar3.setWidth(cell>=0?static_cast<int16_t>((cell>5000?5000:cell)*120/5000):0);CellBar3.invalidate();
    pack.value(value,sizeof(value),"c5_mv"," V",1000,3);if(pack.get("c5_mv",cell)&&cell==-1)strcpy(value,"UNUSED");
    lab_show(CellValue4,CellValue4Buffer,CELLVALUE4_SIZE,value,lab_cyan());
    CellBar4.setColor(lab_cyan());CellBar4.setVisible(pack.get("c5_mv",cell)&&cell>=0);CellBar4.setWidth(cell>=0?static_cast<int16_t>((cell>5000?5000:cell)*120/5000):0);CellBar4.invalidate();

}
void ScreenBatteryView::refreshTelemetry() {notify(g4_simple(psu_g4(),"STATUS",0)?"Refresh requested - waiting for T / TB / TC":"Command queue full");}

void ScreenBatteryView::setupTheme()
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
    theme.panel(MetricCard0);
    theme.text(MetricLabel0);
    theme.text(Metric0);
    theme.panel(MetricCard1);
    theme.text(MetricLabel1);
    theme.text(Metric1);
    theme.panel(MetricCard2);
    theme.text(MetricLabel2);
    theme.text(Metric2);
    theme.panel(MetricCard3);
    theme.text(MetricLabel3);
    theme.text(Metric3);
    theme.text(CellsHeading);
    theme.panel(CellCard0);
    theme.text(CellLabel0);
    theme.text(CellValue0);
    theme.box(CellBar0,ui::ACCENT);
    theme.panel(CellCard1);
    theme.text(CellLabel1);
    theme.text(CellValue1);
    theme.box(CellBar1,ui::ACCENT);
    theme.panel(CellCard2);
    theme.text(CellLabel2);
    theme.text(CellValue2);
    theme.box(CellBar2,ui::ACCENT);
    theme.panel(CellCard3);
    theme.text(CellLabel3);
    theme.text(CellValue3);
    theme.box(CellBar3,ui::ACCENT);
    theme.panel(CellCard4);
    theme.text(CellLabel4);
    theme.text(CellValue4);
    theme.box(CellBar4,ui::ACCENT);
    theme.text(LeftDetails);
    theme.text(RightDetails);
    theme.text(Future);
    theme.apply();
}
