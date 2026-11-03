#ifndef SCREENBATTERYVIEW_HPP
#define SCREENBATTERYVIEW_HPP
#include <gui_generated/screenbattery_screen/ScreenBatteryViewBase.hpp>
#include <gui/screenbattery_screen/ScreenBatteryPresenter.hpp>
#include <stdint.h>
#include <gui/common/BatteryTrendWidget.hpp>
#include <gui/common/Kit.hpp>
class ScreenBatteryView : public ScreenBatteryViewBase {
public:
    void setupTheme();
    ScreenBatteryView();
    virtual ~ScreenBatteryView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void refreshTelemetry();
    virtual void showBattery();
    virtual void openBms();
    virtual void showCharger();
    virtual void showEnergy();
    virtual void openUsb();
protected:
    BatteryTrendWidget trend;
    unsigned page;
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
    void layout();
    kit::Header header;
    kit::Panel socCard, flowCard, healthCard, cellsCard, chargerCard, energyCard;
    kit::Value socValue, packPower, packVolts, packAmps;
    kit::Label socCaption, socDetail, flowCaption, healthCaption, cellsCaption, chargerCaption, chargerPhase, energyCaption;
    kit::Label healthKey[4], healthValue[4], cellName[5];
    kit::Segments socBar;
    kit::Bar chargeBar;
    kit::Level cellLevel[5];
    kit::Chip chgChip, dsgChip, faultChip;
    kit::Rows chargerRows;
};
#endif
