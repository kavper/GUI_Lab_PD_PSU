#include <gui/common/UiTheme.hpp>
#include <gui/screenbms_screen/ScreenBmsView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/TelemetryData.hpp>
ScreenBmsView::ScreenBmsView() : divider(0), noticeTicks(0) {notice[0]=0;}
void ScreenBmsView::setupScreen() {ScreenBmsViewBase::setupScreen(); refresh();
    setupTheme();
}
void ScreenBmsView::tearDownScreen() {ScreenBmsViewBase::tearDownScreen();}
void ScreenBmsView::handleTickEvent() {if(noticeTicks)--noticeTicks;if(++divider>=2){divider=0;refresh();}}
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
void ScreenBmsView::refreshTelemetry() {notify(g4_simple(psu_g4(),"STATUS",0)?"PING sent - telemetry streams automatically":"Command queue full");}

void ScreenBmsView::setupTheme()
{
    ui::ThemeScreen& theme=ui::ThemeScreen::get();
    theme.begin(*this);
    theme.box(LabBackground,ui::BACKGROUND);
    theme.box(LabHeader,ui::SURFACE);
    theme.box(ThemeHeaderDivider,ui::BORDER);
    theme.text(PageTitle);
    theme.text(PageFeedback);
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
    theme.panel(MetricCard4);
    theme.text(MetricLabel4);
    theme.text(Metric4);
    theme.panel(LeftCard);
    theme.text(LeftHeading);
    theme.text(LeftDetails);
    theme.panel(RightCard);
    theme.text(RightHeading);
    theme.text(RightDetails);
    theme.button(InitButton,ui::NORMAL);
    theme.button(RefreshButton,ui::NORMAL);
    theme.button(AllOffButton,ui::DANGER);
    theme.apply();
}
