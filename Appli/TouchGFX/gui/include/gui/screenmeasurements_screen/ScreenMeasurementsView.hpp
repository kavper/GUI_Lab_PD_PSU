#ifndef SCREENMEASUREMENTSVIEW_HPP
#define SCREENMEASUREMENTSVIEW_HPP
#include <gui_generated/screenmeasurements_screen/ScreenMeasurementsViewBase.hpp>
#include <gui/screenmeasurements_screen/ScreenMeasurementsPresenter.hpp>
#include <stdint.h>
#include <gui/common/Kit.hpp>
// Output path: input -> buck-boost -> DCDC rail -> LDO -> output flow, then
// switching stage, rail regulation and LDO efficiency cards.
class ScreenMeasurementsView : public ScreenMeasurementsViewBase {
public:
    ScreenMeasurementsView();
    virtual ~ScreenMeasurementsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void refreshTelemetry();
protected:
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
    void layout();
    void onClick(const kit::Btn&) { refreshTelemetry(); }
    kit::Header header;
    kit::Card stage[5], switching, regulation, efficiency;
    kit::Value stageValue[5], stageValue2, effValue;
    kit::Label stageSub[5][2], arrow[4];
    kit::Bar dutyBar;
    kit::Rows switchRows, regRows, effRows;
    kit::Btn refreshBtn;
    touchgfx::Callback<ScreenMeasurementsView, const kit::Btn&> clicked;
};
#endif
