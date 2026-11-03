#ifndef SCREENUSBPDVIEW_HPP
#define SCREENUSBPDVIEW_HPP
#include <gui_generated/screenusbpd_screen/ScreenUsbPdViewBase.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdPresenter.hpp>
#include <stdint.h>
#include <gui/common/PowerGaugeScale.hpp>
#include <gui/common/Kit.hpp>
// USB-C / PD: port measurements and contract, partner PDO table, role
// selection and the battery power-flow gauge.
class ScreenUsbPdView : public ScreenUsbPdViewBase {
public:
    ScreenUsbPdView();
    virtual ~ScreenUsbPdView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void navigateBack();
    void saveSwipeNotice(char* text, uint16_t& ticks) const;
    void restoreSwipeNotice(const char* text, uint16_t ticks);
    virtual void allOff();
    virtual void roleAuto();
    virtual void roleSink();
    virtual void roleSource();
    virtual void refreshTelemetry();
protected:
    enum { PDOS = 7 };
    PowerGaugeScale gaugeScale;
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
    void layout();
    void onClick(const kit::Btn& b);
    kit::Header header;
    kit::Card portCard, offersCard, flowCard;
    kit::Chip contractChip;
    kit::Value volts, amps, power, flowPower;
    kit::Label voltsSub, ampsSub, powerOf, percent, statCaption[3], statValue[3];
    kit::Bar utilisation, flowBar;
    kit::Panel segment, pdoMark;
    kit::Btn role[3];
    kit::Label tableHead[5], pdoCell[PDOS][5], rdoLine, flowLeft, flowMid, flowRight;
    kit::Fill flowZero;
    touchgfx::Callback<ScreenUsbPdView, const kit::Btn&> clicked;
};
#endif
