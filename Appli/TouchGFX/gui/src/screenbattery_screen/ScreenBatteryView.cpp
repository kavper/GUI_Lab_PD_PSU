#include <gui/common/UiTheme.hpp>
#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <images/BitmapDatabase.hpp>
ScreenBatteryView::ScreenBatteryView():page(0),divider(0),noticeTicks(0){notice[0]=0;}
void ScreenBatteryView::setupScreen(){ScreenBatteryViewBase::setupScreen();trend.setPosition(32,306,736,90);add(trend);setupTheme();refresh();}
void ScreenBatteryView::tearDownScreen(){ScreenBatteryViewBase::tearDownScreen();}
void ScreenBatteryView::handleTickEvent(){if(static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->isScreenTransitionActive())return;if(noticeTicks)--noticeTicks;if(++divider>=6){divider=0;refresh();}}
void ScreenBatteryView::notify(const char* text){snprintf(notice,sizeof(notice),"%s",text);noticeTicks=180;refresh();}
void ScreenBatteryView::allOff(){notify(psu_app_power_shutdown()?"POWER OFF requested / wake with TS2":"POWER OFF request failed");}
void ScreenBatteryView::showBattery(){page=0;refresh();getRootContainer().invalidate();}
void ScreenBatteryView::showCharger(){page=1;refresh();getRootContainer().invalidate();}
void ScreenBatteryView::showEnergy(){page=2;refresh();getRootContainer().invalidate();}
void ScreenBatteryView::openBms(){static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->gotoScreenBmsScreenWipeTransitionWest();}
void ScreenBatteryView::openUsb(){static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->gotoScreenUsbPdScreenWipeTransitionWest();}
void ScreenBatteryView::refreshTelemetry(){notify(g4_simple(psu_g4(),"STATUS",0)?"Telemetry requested":"Command queue full");}
void ScreenBatteryView::refresh(){
    const touchgfx::Bitmap normal(BITMAP_BATTERY_TAB_REL_180X42_ID),selected(BITMAP_BATTERY_TAB_SEL_180X42_ID);
    BatteryTab.setBitmaps(page==0?selected:normal,selected);ChargerTab.setBitmaps(page==1?selected:normal,selected);EnergyTab.setBitmaps(page==2?selected:normal,selected);
    TelemetryRecord pack(G4_RECORD_TB);const bool fresh=pack.fresh;
    pack.fresh=pack.is("bms",1)&&pack.is("sample",1);
    int64_t mv=0,ma=0;const bool valid=pack.get("pack_mv",mv)&&pack.get("i_pack_ma",ma)&&mv>=0&&mv<=100000&&ma>=-100000&&ma<=100000;
    char value[100];
    lab_show(FlowState,FlowStateBuffer,FLOWSTATE_SIZE,!valid?"No valid sample":ma<0?"Discharging":ma>0?"Charging":"Idle",lab_text());
    const int64_t mw=valid?mv*ma/1000:0,absMw=mw<0?-mw:mw;
    if(valid)snprintf(value,sizeof(value),"%ld.%02ld W",(long)(absMw/1000),(long)(absMw%1000/10));else strcpy(value,"-- W");
    lab_show(PackPower,PackPowerBuffer,PACKPOWER_SIZE,value,valid?lab_cyan():lab_muted());
    pack.value(value,sizeof(value),"pack_mv"," V",1000,3);lab_show(PackVoltage,PackVoltageBuffer,PACKVOLTAGE_SIZE,value,lab_text());
    pack.value(value,sizeof(value),"i_pack_ma"," A",1000,3);lab_show(PackCurrent,PackCurrentBuffer,PACKCURRENT_SIZE,value,lab_text());
    char soc[50];pack.value(soc,sizeof(soc),"soc_permille"," %",10,1);snprintf(value,sizeof(value),"SOC %s",soc);lab_show(PackSoc,PackSocBuffer,PACKSOC_SIZE,value,lab_muted());
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,valid?"Internal pack / measured flow":"Internal pack / waiting for valid BMS data",lab_muted());
    touchgfx::Box* cards[]={&CellCard0,&CellCard1,&CellCard2,&CellCard3,&CellCard4};
    touchgfx::TextArea* labels[]={&CellLabel0,&CellLabel1,&CellLabel2,&CellLabel3,&CellLabel4};
    touchgfx::TextArea* readings[]={&CellValue0,&CellValue1,&CellValue2,&CellValue3,&CellValue4};
    touchgfx::TextArea* states[]={&CellStatus0,&CellStatus1,&CellStatus2,&CellStatus3,&CellStatus4};
    touchgfx::Unicode::UnicodeChar* rb[]={CellValue0Buffer,CellValue1Buffer,CellValue2Buffer,CellValue3Buffer,CellValue4Buffer};
    touchgfx::Unicode::UnicodeChar* sb[]={CellStatus0Buffer,CellStatus1Buffer,CellStatus2Buffer,CellStatus3Buffer,CellStatus4Buffer};
    int64_t cells[5]={0},balance=0;bool available[5]={false};int64_t lo=100000,hi=0;unsigned active=0;
    pack.get("balance_mask",balance);
    for(unsigned i=0;i<5;i++){char key[12];snprintf(key,sizeof(key),"c%u_mv",i+1);available[i]=pack.get(key,cells[i]);if(available[i]&&cells[i]>0&&cells[i]<=5000){if(cells[i]<lo)lo=cells[i];if(cells[i]>hi)hi=cells[i];++active;}}
    for(unsigned i=0;i<5;i++){
        cards[i]->setVisible(page==0);labels[i]->setVisible(page==0);readings[i]->setVisible(page==0);states[i]->setVisible(page==0);
        const bool usable=available[i]&&cells[i]>0&&cells[i]<=5000;const bool unused=available[i]&&cells[i]==-1;
        if(usable)snprintf(value,sizeof(value),"%ld.%03ld V",(long)(cells[i]/1000),(long)(cells[i]%1000));else strcpy(value,unused?"UNUSED":"-- V");
        lab_show(*readings[i],rb[i],100,value,usable?lab_text():lab_muted());
        const char* status=unused?"INACTIVE":!usable?"NO DATA":(balance&(1U<<i))?"BALANCING":cells[i]==lo&&cells[i]==hi?"MIN / MAX":cells[i]==lo?"LOWEST":cells[i]==hi?"HIGHEST":"";
        lab_show(*states[i],sb[i],100,status,usable&&(balance&(1U<<i))?lab_green():lab_muted());
    }
    CellsHeading.setVisible(page==0);CellSpread.setVisible(page==0);
    if(active)snprintf(value,sizeof(value),"Cell spread %ld mV | Min %ld.%03ld V / Max %ld.%03ld V | %u active",(long)(hi-lo),(long)(lo/1000),(long)(lo%1000),(long)(hi/1000),(long)(hi%1000),active);
    else strcpy(value,"Cell spread -- | waiting for valid physical channel samples");
    lab_show(CellSpread,CellSpreadBuffer,CELLSPREAD_SIZE,value,lab_text());
    lab_show(StatusHint,StatusHintBuffer,STATUSHINT_SIZE,noticeTicks?notice:!fresh?"BMS offline / stale telemetry":!valid?"BMS detected / no valid ADC sample":pack.is("cfg",1)?"BMS ready / five physical inputs":"BMS detected / configuration pending",lab_muted());
    ChargerPanel.setVisible(page==1);ChargerData.setVisible(page==1);ChargerStatus.setVisible(page==1);
    if(page==1){TelemetryData charger;telemetry_page(PAGE_CHARGER,charger);lab_show(ChargerData,ChargerDataBuffer,CHARGERDATA_SIZE,charger.left,lab_text());lab_show(ChargerStatus,ChargerStatusBuffer,CHARGERSTATUS_SIZE,charger.right,lab_text());}
    EnergyPanel.setVisible(page==2);EnergyValue.setVisible(page==2);EnergyHint.setVisible(page==2);trend.setVisible(page==2);
    if(page==2){PsuSnapshot snap;psu_snapshot(&snap);if(snap.battery_energy_valid)snprintf(value,sizeof(value),"%+.3f Wh",(double)snap.battery_energy_uwms/3600000000000.0);else strcpy(value,"-- Wh");lab_show(EnergyValue,EnergyValueBuffer,ENERGYVALUE_SIZE,value,lab_cyan());snprintf(value,sizeof(value),"Signed since startup / +/- %u W",trend.rangeMw()/1000);lab_show(EnergyHint,EnergyHintBuffer,ENERGYHINT_SIZE,value,lab_muted());trend.invalidate();}
    ui::ThemeScreen::get().sync();
}
void ScreenBatteryView::setupTheme(){
    auto& theme=ui::ThemeScreen::get();theme.begin(*this);theme.box(LabBackground,ui::BACKGROUND);theme.box(LabHeader,ui::SURFACE);theme.box(ThemeHeaderDivider,ui::BORDER);
    theme.text(PageTitle);theme.text(PageFeedback);theme.text(StatusHint);theme.text(CellsHeading);theme.text(CellSpread);
    theme.button(AllOffButton,ui::DANGER);theme.button(BackButton);theme.button(BatteryTab);theme.button(DetailsTab);theme.button(ChargerTab);theme.button(EnergyTab);theme.button(UsbButton);
    theme.panel(SummaryCard);theme.text(FlowState);theme.text(PackPower);theme.text(PackVoltage);theme.text(PackCurrent);theme.text(PackSoc);
    theme.panel(CellCard0);theme.panel(CellCard1);theme.panel(CellCard2);theme.panel(CellCard3);theme.panel(CellCard4);
    theme.text(CellLabel0);theme.text(CellLabel1);theme.text(CellLabel2);theme.text(CellLabel3);theme.text(CellLabel4);
    theme.text(CellValue0);theme.text(CellValue1);theme.text(CellValue2);theme.text(CellValue3);theme.text(CellValue4);
    theme.text(CellStatus0);theme.text(CellStatus1);theme.text(CellStatus2);theme.text(CellStatus3);theme.text(CellStatus4);
    theme.panel(ChargerPanel);theme.text(ChargerData);theme.text(ChargerStatus);theme.panel(EnergyPanel);theme.text(EnergyValue);theme.text(EnergyHint);theme.apply();
}
