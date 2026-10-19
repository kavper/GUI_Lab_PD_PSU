#include <gui/common/UiTheme.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/UsbSession.hpp>
#include <gui/common/UsbPdObjects.hpp>
ScreenUsbPdView::ScreenUsbPdView() : gaugePosition(0), gaugeTarget(0), gaugeValid(false), divider(0), noticeTicks(0) {notice[0]=0;}
void ScreenUsbPdView::setupScreen() {ScreenUsbPdViewBase::setupScreen(); static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::USB_PD); refresh();
    setupTheme();
}
void ScreenUsbPdView::tearDownScreen() {static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::OTHER); ScreenUsbPdViewBase::tearDownScreen();}
void ScreenUsbPdView::handleTickEvent() {if(static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->isScreenTransitionActive())return;animateGauge();if(noticeTicks)--noticeTicks;if(++divider>=2){divider=0;refresh();}}
void ScreenUsbPdView::notify(const char* text) {snprintf(notice,sizeof(notice),"%s",text);noticeTicks=180;refresh();}
void ScreenUsbPdView::allOff() {psu_app_power_shutdown();notify("POWER OFF requested - BMS shutdown / wake with TS2");}
void ScreenUsbPdView::refresh() {
    TelemetryData data;
    telemetry_page(PAGE_USB,data);
    lab_show(Metric0,Metric0Buffer,METRIC0_SIZE,data.metric[0],data.fresh?lab_text():lab_muted());
    lab_show(Metric1,Metric1Buffer,METRIC1_SIZE,data.metric[1],data.fresh?lab_text():lab_muted());
    lab_show(Metric2,Metric2Buffer,METRIC2_SIZE,data.metric[2],data.fresh?lab_text():lab_muted());
    lab_show(Metric3,Metric3Buffer,METRIC3_SIZE,data.metric[3],data.fresh?lab_text():lab_muted());
    TelemetryRecord usb(G4_RECORD_TC), battery(G4_RECORD_TB);
    const TelemetryRecord pd=usb;
    // Contract limits and ADC measurements are separate physical quantities.
    int64_t role=0, voltage=0, current=0;
    int64_t contractRole=0,pdMv=0,pdMa=0;
    const bool attached=usb.is("plug",1);
    usb.get("pd_role",contractRole);role=contractRole;
    if(role!=1&&role!=2)usb.get("role",role);
    const bool contractValid=attached&&(contractRole==1||contractRole==2)&&usb.get("pd_mv",pdMv)&&usb.get("pd_ma",pdMa)&&pdMv>0&&pdMa>0;
    lab_show(Metric3,Metric3Buffer,METRIC3_SIZE,!usb.fresh?"OFFLINE":!attached?"DETACHED":role==1?"SINK":role==2?"SOURCE":"WAIT",attached?ui::Theme::color(ui::ACCENT):lab_muted());
    if(!contractValid) {
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
    char load[100];int64_t limitMv=0,limitMa=0;unsigned percent=0;
    const bool loadValid=inputValid&&usb.get("bq_vbus_mv",voltage)&&voltage>=0&&voltage<=100000&&
        usb.get("pd_mv",limitMv)&&usb.get("pd_ma",limitMa)&&limitMv>0&&limitMa>0&&limitMv<=100000&&limitMa<=10000;
    if(loadValid){percent=(unsigned)(((voltage*current)*100+limitMv*limitMa/2)/(limitMv*limitMa));snprintf(load,sizeof(load),"Contract utilization: %u%%",percent);}
    else snprintf(load,sizeof(load),"%s",!attached?"Connect a USB-C device":role==2?"Outgoing current: sensor unavailable":!contractValid?"No negotiated PD power limit":"Waiting for valid input ADC");
    lab_show(ContractDetail,ContractDetailBuffer,CONTRACTDETAIL_SIZE,load,lab_muted());
    const int fill=loadValid?(percent>100?276:percent*276/100):0;
    if(ContractLoadFill.getWidth()!=fill||ContractLoadFill.isVisible()!=loadValid){ContractLoadFill.invalidate();ContractLoadFill.setWidth(fill);ContractLoadFill.setVisible(loadValid);ContractLoadFill.invalidate();}
    const auto loadColor=ui::Theme::color(percent>100?ui::CAUTION:ui::ACCENT);
    if(ContractLoadFill.getColor()!=loadColor){ContractLoadFill.setColor(loadColor);ContractLoadFill.invalidate();}
    const UsbSession& session=usbSession();
    if(session.hasSamples()){
        const uint64_t cwh=session.energyMwMs/36000000ULL;
        snprintf(load,sizeof(load),"Received %lu.%02lu Wh\nValid time %lu:%02lu\nPeak input %lu.%02lu W",(unsigned long)(cwh/100),(unsigned long)(cwh%100),(unsigned long)(session.elapsedMs/60000),(unsigned long)(session.elapsedMs/1000%60),(unsigned long)(session.peakMw/1000),(unsigned long)(session.peakMw%1000/10));
    }else snprintf(load,sizeof(load),"Received -- Wh\nValid time --\n%s",attached&&role==2?"TX energy: no sensor":"Waiting for input samples");
    lab_show(SessionSummary,SessionSummaryBuffer,SESSIONSUMMARY_SIZE,load,lab_muted());
    G4PdCapabilities caps;g4_pd_capabilities_snapshot(psu_g4(),&caps);
    int64_t generation=0;pd.get("attach_gen",generation);
    const bool currentCaps=ui::pdCapabilitiesCurrent(caps,psu_app_now(),attached,(unsigned)role,(unsigned)generation);
    lab_show(CapabilitiesHeading,CapabilitiesHeadingBuffer,CAPABILITIESHEADING_SIZE,currentCaps&&(caps.flags&8u)?"DEVICE / ADVERTISED SOURCE PDO":"PARTNER / SOURCE CAPABILITIES",ui::Theme::color(ui::ACCENT));
    char rows[640]="",request[200];unsigned selected=0;
    if(currentCaps&&(caps.flags&2u)&&caps.count){
        selected=ui::selectedPdObject(caps);
        for(unsigned i=0;i<caps.count;++i){char row[80];ui::formatPdRow(row,sizeof(row),caps.pdo[i],i+1,selected==i+1);size_t used=strlen(rows);snprintf(rows+used,sizeof(rows)-used,"%s%s",i?"\n":"",row);}
    }else snprintf(rows,sizeof(rows),"%s",!attached?"Connect a USB-C partner\nPDOs belong to the current cable session":!caps.valid?"PDO list not reported by G4\nRequires extended PD telemetry":!currentCaps?"Waiting for current-session PDOs":"Waiting for source capabilities");
    lab_show(PdoRows,PdoRowsBuffer,PDOROWS_SIZE,rows,lab_text());
    const int rowY=selected?250+(selected-1)*18:250;
    const auto rowColor=ui::Theme::color(ui::ACCENT_SOFT);
    if(PdoSelected.isVisible()!=(selected!=0)||PdoSelected.getY()!=rowY||PdoSelected.getColor()!=rowColor){PdoSelected.invalidate();PdoSelected.setVisible(selected!=0);PdoSelected.setY(rowY);PdoSelected.setColor(rowColor);PdoSelected.invalidate();}
    if(currentCaps)ui::formatPdRequest(request,sizeof(request),caps);
    else snprintf(request,sizeof(request),"RDO unavailable\n%s",attached?"Waiting for current session":"No PD contract");
    lab_show(RequestDetails,RequestDetailsBuffer,REQUESTDETAILS_SIZE,request,lab_text());
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
    theme.panel(SessionCard);theme.text(SessionHeading,ui::ACCENT);
    theme.panel(CapabilitiesCard);theme.text(CapabilitiesHeading,ui::ACCENT);
    theme.panel(RequestCard);theme.text(RequestHeading,ui::ACCENT);
    theme.text(PdoRows);theme.text(RequestDetails);
    theme.panel(ContractCard);
    theme.panel(MeasuredCard);
    theme.panel(BatteryCard);
    theme.text(ContractDetail,ui::MUTED);
    theme.text(SessionSummary,ui::MUTED);
    theme.box(ContractLoadTrack,ui::BORDER);
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

void ScreenUsbPdView::navigateBack(){static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->navigateBack();}
