#include <gui/common/UiTheme.hpp>
#include <gui/screenprotection_screen/ScreenProtectionView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/TelemetryData.hpp>
ScreenProtectionView::ScreenProtectionView() : divider(0), noticeTicks(0) {notice[0]=0;}
void ScreenProtectionView::setupScreen() {ScreenProtectionViewBase::setupScreen(); refresh();
    setupTheme();
}
void ScreenProtectionView::tearDownScreen() {ScreenProtectionViewBase::tearDownScreen();}
void ScreenProtectionView::handleTickEvent() {if(noticeTicks)--noticeTicks;if(++divider>=2){divider=0;refresh();}}
void ScreenProtectionView::notify(const char* text) {snprintf(notice,sizeof(notice),"%s",text);noticeTicks=180;refresh();}
void ScreenProtectionView::allOff() {psu_app_power_shutdown();notify("POWER OFF requested - BMS shutdown / wake with TS2");}
void ScreenProtectionView::refresh() {
    TelemetryData data;
    telemetry_page(PAGE_PROTECTION,data);
    lab_show(Metric0,Metric0Buffer,METRIC0_SIZE,data.metric[0],data.fresh?lab_text():lab_muted());
    lab_show(Metric1,Metric1Buffer,METRIC1_SIZE,data.metric[1],data.fresh?lab_text():lab_muted());
    lab_show(Metric2,Metric2Buffer,METRIC2_SIZE,data.metric[2],data.fresh?lab_text():lab_muted());
    lab_show(Metric3,Metric3Buffer,METRIC3_SIZE,data.metric[3],data.fresh?lab_text():lab_muted());
    lab_show(LeftDetails,LeftDetailsBuffer,LEFTDETAILS_SIZE,data.left,lab_text());
    lab_show(RightDetails,RightDetailsBuffer,RIGHTDETAILS_SIZE,data.right,lab_text());
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,noticeTicks?notice:data.status,data.fresh?lab_muted():lab_amber());
    lab_enable(ClearButton,data.fresh && data.fault);
    lab_enable(RefreshButton,true);
}
void ScreenProtectionView::clearFault() {notify(psu_app_clear_fault()?"CLR requested; confirm that reported faults clear":"No PSU fault to clear");}
void ScreenProtectionView::refreshTelemetry() {notify(g4_simple(psu_g4(),"STATUS",0)?"PING sent - telemetry streams automatically":"Command queue full");}

void ScreenProtectionView::setupTheme()
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
    theme.button(ClearButton,ui::NORMAL);
    theme.button(RefreshButton,ui::NORMAL);
    theme.button(AllOffButton,ui::DANGER);
    theme.apply();
}
