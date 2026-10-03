#include <gui/common/UiTheme.hpp>
#include <gui/screenmeasurements_screen/ScreenMeasurementsView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/TelemetryData.hpp>
ScreenMeasurementsView::ScreenMeasurementsView() : divider(0), noticeTicks(0) {notice[0]=0;}
void ScreenMeasurementsView::setupScreen() {ScreenMeasurementsViewBase::setupScreen(); refresh();
    setupTheme();
}
void ScreenMeasurementsView::tearDownScreen() {ScreenMeasurementsViewBase::tearDownScreen();}
void ScreenMeasurementsView::handleTickEvent() {if(noticeTicks)--noticeTicks;if(++divider>=8){divider=0;refresh();}}
void ScreenMeasurementsView::notify(const char* text) {snprintf(notice,sizeof(notice),"%s",text);noticeTicks=180;refresh();}
void ScreenMeasurementsView::allOff() {psu_app_shutdown();notify("PSU OFF requested - stopping LDO, DCDC and automation");}
void ScreenMeasurementsView::refresh() {
    TelemetryData data;
    telemetry_page(PAGE_DCDC,data);
    lab_show(Metric0,Metric0Buffer,METRIC0_SIZE,data.metric[0],data.fresh?lab_text():lab_muted());
    lab_show(Metric1,Metric1Buffer,METRIC1_SIZE,data.metric[1],data.fresh?lab_text():lab_muted());
    lab_show(Metric2,Metric2Buffer,METRIC2_SIZE,data.metric[2],data.fresh?lab_text():lab_muted());
    lab_show(Metric3,Metric3Buffer,METRIC3_SIZE,data.metric[3],data.fresh?lab_text():lab_muted());
    lab_show(LeftDetails,LeftDetailsBuffer,LEFTDETAILS_SIZE,data.left,lab_text());
    lab_show(RightDetails,RightDetailsBuffer,RIGHTDETAILS_SIZE,data.right,lab_text());
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,noticeTicks?notice:data.status,data.fresh?lab_muted():lab_amber());
    lab_enable(RefreshButton,true);
}
void ScreenMeasurementsView::refreshTelemetry() {notify(g4_simple(psu_g4(),"STATUS",0)?"Refresh requested - waiting for T / TB / TC":"Command queue full");}

void ScreenMeasurementsView::setupTheme()
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
    theme.panel(LeftCard);
    theme.text(LeftHeading);
    theme.text(LeftDetails);
    theme.panel(RightCard);
    theme.text(RightHeading);
    theme.text(RightDetails);
    theme.button(RefreshButton,ui::NORMAL);
    theme.button(AllOffButton,ui::DANGER);
    theme.apply();
}
