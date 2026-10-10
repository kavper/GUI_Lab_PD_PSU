#include <gui/common/UiTheme.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/TelemetryData.hpp>
ScreenUsbPdView::ScreenUsbPdView() : gaugePosition(0), gaugeTarget(0), gaugeValid(false), divider(0), noticeTicks(0) {notice[0]=0;}
void ScreenUsbPdView::setupScreen() {ScreenUsbPdViewBase::setupScreen(); static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::USB_PD); refresh();
    setupTheme();
}
void ScreenUsbPdView::tearDownScreen() {static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::OTHER); ScreenUsbPdViewBase::tearDownScreen();}
void ScreenUsbPdView::handleTickEvent() {if(static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->isScreenTransitionActive())return;animateGauge();if(noticeTicks)--noticeTicks;if(++divider>=2){divider=0;refresh();}}
void ScreenUsbPdView::notify(const char* text) {snprintf(notice,sizeof(notice),"%s",text);noticeTicks=180;refresh();}
void ScreenUsbPdView::allOff() {psu_app_shutdown();notify("PSU OFF requested - stopping LDO, DCDC and automation");}
void ScreenUsbPdView::refresh() {
    TelemetryData data;
    telemetry_page(PAGE_USB,data);
    lab_show(Metric0,Metric0Buffer,METRIC0_SIZE,data.metric[0],data.fresh?lab_text():lab_muted());
    lab_show(Metric1,Metric1Buffer,METRIC1_SIZE,data.metric[1],data.fresh?lab_text():lab_muted());
    lab_show(Metric2,Metric2Buffer,METRIC2_SIZE,data.metric[2],data.fresh?lab_text():lab_muted());
    lab_show(Metric3,Metric3Buffer,METRIC3_SIZE,data.metric[3],data.fresh?lab_text():lab_muted());
    TelemetryRecord usb(G4_RECORD_TC), battery(G4_RECORD_TB);
    // Contract limits and ADC measurements are separate physical quantities.
    int64_t role=0, voltage=0, current=0;
    const bool attached=usb.is("plug",1) && usb.get("pd_role",role) && (role==1||role==2);
    if(!attached) {
        lab_show(Metric0,Metric0Buffer,METRIC0_SIZE,"--",lab_muted());
        lab_show(Metric1,Metric1Buffer,METRIC1_SIZE,"--",lab_muted());
        lab_show(Metric2,Metric2Buffer,METRIC2_SIZE,"--",lab_muted());
    }
    char measuredVoltage[32], measuredCurrent[32], measuredPower[32]="--";
    usb.value(measuredVoltage,sizeof(measuredVoltage),"tps_vbus_mv"," V",1000,3);
    usb.fresh=usb.fresh && usb.is("bq_ok",1);
    // Input-current ADC cannot establish the outgoing USB current in OTG mode.
    const bool inputValid=attached && role==1 && usb.get("bq_iin_ma",current) && current>=0 && current<=10000;
    if(inputValid)usb.value(measuredCurrent,sizeof(measuredCurrent),"bq_iin_ma"," A",1000,3);
    else snprintf(measuredCurrent,sizeof(measuredCurrent),"--");
    if(inputValid && usb.get("bq_vbus_mv",voltage) && voltage>=0 && voltage<=100000) {
        const int64_t mw=voltage*current/1000;
        snprintf(measuredPower,sizeof(measuredPower),"%ld.%02ld W",(long)(mw/1000),(long)((mw%1000)/10));
    }
    lab_show(Measured0,Measured0Buffer,MEASURED0_SIZE,measuredVoltage,lab_text());
    lab_show(Measured1,Measured1Buffer,MEASURED1_SIZE,measuredCurrent,inputValid?lab_text():lab_muted());
    lab_show(Measured2,Measured2Buffer,MEASURED2_SIZE,measuredPower,inputValid?lab_text():lab_muted());
    battery.fresh=battery.fresh && battery.is("sample",1) && battery.is("bms",1);
    const bool wasValid=gaugeValid;
    gaugeValid=battery.get("pack_mv",voltage) && battery.get("i_pack_ma",current)
        && voltage>=0 && voltage<=100000 && current>=-100000 && current<=100000;
    char power[32]="--", detail[80]="Waiting for valid BMS sample";
    if(gaugeValid) {
        const int64_t mw=voltage*current/1000, magnitude=mw<0?-mw:mw;
        snprintf(power,sizeof(power),"%s%ld.%02ld W",mw<0?"-":mw>0?"+":"",(long)(magnitude/1000),(long)((magnitude%1000)/10));
        snprintf(detail,sizeof(detail),"%ld.%03ld V / %s%ld.%03ld A",(long)(voltage/1000),(long)(voltage%1000),current<0?"-":"",(long)((current<0?-current:current)/1000),(long)((current<0?-current:current)%1000));
        const uint32_t oldRange=gaugeScale.range();
        gaugeScale.update(mw,psu_app_now(),true);
        if(oldRange!=gaugeScale.range())
            gaugePosition=static_cast<int16_t>(static_cast<int64_t>(gaugePosition)*oldRange/gaugeScale.range());
        gaugeTarget=gaugeScale.position(mw);
        if(!wasValid)gaugePosition=gaugeTarget;
        else if((gaugePosition<0 && gaugeTarget>=0)||(gaugePosition>0 && gaugeTarget<=0))gaugePosition=0;
    }
    else {gaugeTarget=gaugePosition=0;gaugeScale.update(0,psu_app_now(),false);}
    char rangeLabel[32];
    const unsigned long rangeW=gaugeScale.range()/1000U;
    snprintf(rangeLabel,sizeof(rangeLabel),"-%lu W / DISCHARGING",rangeW);
    lab_show(DischargeLabel,DischargeLabelBuffer,DISCHARGELABEL_SIZE,rangeLabel,lab_red());
    snprintf(rangeLabel,sizeof(rangeLabel),"CHARGING / +%lu W",rangeW);
    lab_show(ChargeLabel,ChargeLabelBuffer,CHARGELABEL_SIZE,rangeLabel,lab_green());
    snprintf(rangeLabel,sizeof(rangeLabel),"AUTO +/- %lu W",rangeW);
    lab_show(GaugeRangeLabel,GaugeRangeLabelBuffer,GAUGERANGELABEL_SIZE,rangeLabel,lab_muted());
    const touchgfx::colortype flow=ui::Theme::color(!gaugeValid?ui::MUTED:current>0?ui::POSITIVE:current<0?ui::NEGATIVE:ui::TEXT);
    lab_show(BatteryPower,BatteryPowerBuffer,BATTERYPOWER_SIZE,power,flow);
    lab_show(BatteryDetail,BatteryDetailBuffer,BATTERYDETAIL_SIZE,detail,lab_muted());
    GaugeNeedle.setColor(flow); GaugeFill.setColor(flow);
    GaugeNeedle.setVisible(gaugeValid); GaugeFill.setVisible(gaugeValid);
    GaugeNeedle.invalidate();GaugeFill.invalidate();
    GaugeNeedle.setX(398+gaugePosition);
    GaugeFill.setX(gaugePosition<0?400+gaugePosition:400);
    GaugeFill.setWidth(gaugePosition<0?-gaugePosition:gaugePosition?gaugePosition:1);
    GaugeNeedle.invalidate();GaugeFill.invalidate();
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,noticeTicks?notice:data.status,data.fresh?lab_muted():lab_amber());
    lab_enable(AutoButton,data.fresh);
    lab_enable(SinkButton,data.fresh);
    lab_enable(SourceButton,data.fresh);
    lab_enable(RefreshButton,true);
}
void ScreenUsbPdView::roleAuto() {notify(psu_app_usb_role("AUTO",PSU_SRC_LCD)?"AUTO requested; role below is the actual PD contract":"Role request blocked");}
void ScreenUsbPdView::roleSink() {notify(psu_app_usb_role("SINK",PSU_SRC_LCD)?"SINK requested; waiting for the reported role":"Role request blocked");}
void ScreenUsbPdView::roleSource() {notify(psu_app_usb_role("SOURCE",PSU_SRC_LCD)?"SOURCE requested; waiting for the reported role":"Role request blocked");}
void ScreenUsbPdView::refreshTelemetry() {notify(g4_simple(psu_g4(),"STATUS",0)?"PING sent - telemetry streams automatically":"Command queue full");}

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
    theme.panel(ContractCard);
    theme.panel(MeasuredCard);
    theme.panel(BatteryCard);
    theme.text(ContractHeading,ui::ACCENT);
    theme.text(MeasuredHeading,ui::ACCENT);
    theme.text(BatteryHeading,ui::ACCENT);
    theme.text(Metric0);theme.text(Metric1);theme.text(Metric2);theme.text(Metric3);
    theme.text(Measured0);theme.text(Measured1);theme.text(Measured2);
    theme.text(ContractLabel0,ui::MUTED);theme.text(ContractLabel1,ui::MUTED);theme.text(ContractLabel2,ui::MUTED);
    theme.text(MeasuredLabel0,ui::MUTED);theme.text(MeasuredLabel1,ui::MUTED);theme.text(MeasuredLabel2,ui::MUTED);
    theme.text(BatteryDetail,ui::MUTED);
    theme.text(DischargeLabel,ui::NEGATIVE);theme.text(ChargeLabel,ui::POSITIVE);theme.text(ZeroLabel,ui::MUTED);
    theme.text(GaugeRangeLabel,ui::MUTED);
    theme.box(GaugeDischarge,ui::NEGATIVE);theme.box(GaugeCharge,ui::POSITIVE);theme.box(GaugeZero,ui::MUTED);
    theme.box(GaugeNegTick,ui::MUTED);theme.box(GaugePosTick,ui::MUTED);
    GaugeDischarge.setAlpha(70);GaugeCharge.setAlpha(70);
    theme.button(AutoButton,ui::NORMAL);
    theme.button(SinkButton,ui::NORMAL);
    theme.button(SourceButton,ui::NORMAL);
    theme.button(RefreshButton,ui::NORMAL);
    theme.button(AllOffButton,ui::DANGER);
    theme.apply();
    refresh();
}

void ScreenUsbPdView::animateGauge()
{
    if(!gaugeValid)return;
    const int16_t difference=gaugeTarget-gaugePosition;
    if(!difference)return;
    GaugeNeedle.invalidate();GaugeFill.invalidate();
    gaugePosition += difference>0 ? (difference+5)/6 : (difference-5)/6;
    GaugeNeedle.setX(398+gaugePosition);
    GaugeFill.setX(gaugePosition<0?400+gaugePosition:400);
    GaugeFill.setWidth(gaugePosition<0?-gaugePosition:gaugePosition?gaugePosition:1);
    GaugeNeedle.invalidate();GaugeFill.invalidate();
}
