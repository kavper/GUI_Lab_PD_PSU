#include <gui/common/UiTheme.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/TelemetryData.hpp>
ScreenUsbPdView::ScreenUsbPdView() : divider(0), noticeTicks(0) {notice[0]=0;}
void ScreenUsbPdView::setupScreen() {ScreenUsbPdViewBase::setupScreen(); static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::USB_PD); refresh();
    setupTheme();
}
void ScreenUsbPdView::tearDownScreen() {static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::OTHER); ScreenUsbPdViewBase::tearDownScreen();}
void ScreenUsbPdView::handleTickEvent() {if(static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->isScreenTransitionActive())return;if(noticeTicks)--noticeTicks;if(++divider>=8){divider=0;refresh();}}
void ScreenUsbPdView::notify(const char* text) {snprintf(notice,sizeof(notice),"%s",text);noticeTicks=180;refresh();}
void ScreenUsbPdView::allOff() {psu_app_shutdown();notify("PSU OFF requested - stopping LDO, DCDC and automation");}
void ScreenUsbPdView::refresh() {
    TelemetryData data;
    telemetry_page(PAGE_USB,data);
    lab_show(Metric0,Metric0Buffer,METRIC0_SIZE,data.metric[0],data.fresh?lab_text():lab_muted());
    lab_show(Metric1,Metric1Buffer,METRIC1_SIZE,data.metric[1],data.fresh?lab_text():lab_muted());
    lab_show(Metric2,Metric2Buffer,METRIC2_SIZE,data.metric[2],data.fresh?lab_text():lab_muted());
    lab_show(Metric3,Metric3Buffer,METRIC3_SIZE,data.metric[3],data.fresh?lab_text():lab_muted());
    TelemetryRecord usb(G4_RECORD_TC), path(G4_RECORD_T);
    data.left[0]=data.right[0]=0;
    usb.row(data.left,sizeof(data.left),"VBUS measured","tps_vbus_mv"," V",1000,3);
    usb.row(data.left,sizeof(data.left),"Input current","bq_iin_ma"," A",1000,3);
    int64_t volts=0,amps=0;char power[32]="--";
    if(usb.is("bq_ok",1)&&usb.get("bq_vbus_mv",volts)&&usb.get("bq_iin_ma",amps))snprintf(power,sizeof(power),"%ld.%02ld W",(long)(volts*amps/1000000),(long)((volts*amps/10000)%100));
    TelemetryRecord::append(data.left,sizeof(data.left),"Measured input",power);
    usb.flag(data.left,sizeof(data.left),"Cable","plug","ATTACHED","DETACHED");
    path.flag(data.right,sizeof(data.right),"DCDC stage","stage_en");path.flag(data.right,sizeof(data.right),"LDO output","g0_out");
    path.flag(data.right,sizeof(data.right),"LDO permit","permit","ALLOWED","BLOCKED");usb.flag(data.right,sizeof(data.right),"Source OTG","bq_otg");
    lab_show(LeftDetails,LeftDetailsBuffer,LEFTDETAILS_SIZE,data.left,lab_text());
    lab_show(RightDetails,RightDetailsBuffer,RIGHTDETAILS_SIZE,data.right,lab_text());
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,noticeTicks?notice:data.status,data.fresh?lab_muted():lab_amber());
    lab_enable(AutoButton,data.fresh);
    lab_enable(SinkButton,data.fresh);
    lab_enable(SourceButton,data.fresh);
    lab_enable(RefreshButton,true);
}
void ScreenUsbPdView::roleAuto() {notify(psu_app_usb_role("AUTO",PSU_SRC_LCD)?"AUTO requested; role below is the actual PD contract":"Role request blocked");}
void ScreenUsbPdView::roleSink() {notify(psu_app_usb_role("SINK",PSU_SRC_LCD)?"SINK requested; waiting for the reported role":"Role request blocked");}
void ScreenUsbPdView::roleSource() {notify(psu_app_usb_role("SOURCE",PSU_SRC_LCD)?"SOURCE requested; waiting for the reported role":"Role request blocked");}
void ScreenUsbPdView::refreshTelemetry() {notify(g4_simple(psu_g4(),"STATUS",0)?"Refresh requested - waiting for T / TB / TC":"Command queue full");}

void ScreenUsbPdView::saveSwipeNotice(char* text,uint16_t& ticks) const { memcpy(text,notice,sizeof(notice)); ticks=noticeTicks; }
void ScreenUsbPdView::restoreSwipeNotice(const char* text,uint16_t ticks) { memcpy(notice,text,sizeof(notice)); noticeTicks=ticks; refresh(); }

void ScreenUsbPdView::setupTheme()
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
    theme.button(AutoButton,ui::NORMAL);
    theme.button(SinkButton,ui::NORMAL);
    theme.button(SourceButton,ui::NORMAL);
    theme.button(RefreshButton,ui::NORMAL);
    theme.button(AllOffButton,ui::DANGER);
    theme.text(CapabilityNote);
    theme.apply();
}
