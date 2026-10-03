#include <gui/screenpowerpath_screen/ScreenPowerPathView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/TelemetryData.hpp>
ScreenPowerPathView::ScreenPowerPathView() : divider(0), noticeTicks(0) {notice[0]=0;}
void ScreenPowerPathView::setupScreen() {ScreenPowerPathViewBase::setupScreen(); refresh();}
void ScreenPowerPathView::tearDownScreen() {ScreenPowerPathViewBase::tearDownScreen();}
void ScreenPowerPathView::handleTickEvent() {if(noticeTicks)--noticeTicks;if(++divider>=8){divider=0;refresh();}}
void ScreenPowerPathView::notify(const char* text) {snprintf(notice,sizeof(notice),"%s",text);noticeTicks=180;refresh();}
void ScreenPowerPathView::allOff() {psu_app_shutdown();notify("PSU OFF requested - stopping LDO, DCDC and automation");}
void ScreenPowerPathView::refresh() {
    TelemetryData data;
    telemetry_page(PAGE_PATH,data);
    lab_show(Metric0,Metric0Buffer,METRIC0_SIZE,data.metric[0],data.fresh?lab_text():lab_muted());
    lab_show(Metric1,Metric1Buffer,METRIC1_SIZE,data.metric[1],data.fresh?lab_text():lab_muted());
    lab_show(Metric2,Metric2Buffer,METRIC2_SIZE,data.metric[2],data.fresh?lab_text():lab_muted());
    lab_show(Metric3,Metric3Buffer,METRIC3_SIZE,data.metric[3],data.fresh?lab_text():lab_muted());
    lab_show(LeftDetails,LeftDetailsBuffer,LEFTDETAILS_SIZE,data.left,lab_text());
    lab_show(RightDetails,RightDetailsBuffer,RIGHTDETAILS_SIZE,data.right,lab_text());
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,noticeTicks?notice:data.status,data.fresh?lab_muted():lab_amber());
    lab_enable(PermitButton,data.fresh);
    lab_enable(SenseButton,data.fresh);
    lab_enable(RefreshButton,true);
}
void ScreenPowerPathView::togglePermit() {TelemetryRecord r(G4_RECORD_T);int64_t v;if(!r.get("permit",v)){notify("No fresh permit state");return;}notify(g4_permit(psu_g4(),v==0,0)?"Permit change requested; waiting for telemetry":"Request blocked");}
void ScreenPowerPathView::toggleSense() {TelemetryRecord r(G4_RECORD_T);int64_t v;if(!r.get("rem_sense",v)){notify("No fresh sense state");return;}notify(g4_remote(psu_g4(),v==0,0)?"Sense change requested; waiting for telemetry":"Request blocked");}
void ScreenPowerPathView::refreshTelemetry() {notify(g4_simple(psu_g4(),"STATUS",0)?"Refresh requested - waiting for T / TB / TC":"Command queue full");}
