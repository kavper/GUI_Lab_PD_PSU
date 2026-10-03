#include <gui/screenbms_screen/ScreenBmsView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/TelemetryData.hpp>
ScreenBmsView::ScreenBmsView() : divider(0), noticeTicks(0) {notice[0]=0;}
void ScreenBmsView::setupScreen() {ScreenBmsViewBase::setupScreen(); refresh();}
void ScreenBmsView::tearDownScreen() {ScreenBmsViewBase::tearDownScreen();}
void ScreenBmsView::handleTickEvent() {if(noticeTicks)--noticeTicks;if(++divider>=8){divider=0;refresh();}}
void ScreenBmsView::notify(const char* text) {snprintf(notice,sizeof(notice),"%s",text);noticeTicks=180;refresh();}
void ScreenBmsView::allOff() {psu_app_shutdown();notify("PSU OFF requested - stopping LDO, DCDC and automation");}
void ScreenBmsView::refresh() {
    TelemetryData data;
    telemetry_page(PAGE_BMS,data);
    lab_show(Metric0,Metric0Buffer,METRIC0_SIZE,data.metric[0],data.fresh?lab_text():lab_muted());
    lab_show(Metric1,Metric1Buffer,METRIC1_SIZE,data.metric[1],data.fresh?lab_text():lab_muted());
    lab_show(Metric2,Metric2Buffer,METRIC2_SIZE,data.metric[2],data.fresh?lab_text():lab_muted());
    lab_show(Metric3,Metric3Buffer,METRIC3_SIZE,data.metric[3],data.fresh?lab_text():lab_muted());
    lab_show(Metric4,Metric4Buffer,METRIC4_SIZE,data.metric[4],data.fresh?lab_text():lab_muted());
    lab_show(LeftDetails,LeftDetailsBuffer,LEFTDETAILS_SIZE,data.left,lab_text());
    lab_show(RightDetails,RightDetailsBuffer,RIGHTDETAILS_SIZE,data.right,lab_text());
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,noticeTicks?notice:data.status,data.fresh?lab_muted():lab_amber());
    lab_enable(InitButton,data.fresh);
    lab_enable(RefreshButton,true);
}
void ScreenBmsView::initializeBms() {notify(psu_app_bms_cmd("BMS")?"BMS initialization requested; waiting for reported status":"BMS request blocked");}
void ScreenBmsView::refreshTelemetry() {notify(g4_simple(psu_g4(),"STATUS",0)?"Refresh requested - waiting for T / TB / TC":"Command queue full");}
