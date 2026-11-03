#ifndef SCREENPOWERPATHVIEW_HPP
#define SCREENPOWERPATHVIEW_HPP
#include <gui_generated/screenpowerpath_screen/ScreenPowerPathViewBase.hpp>
#include <gui/screenpowerpath_screen/ScreenPowerPathPresenter.hpp>
#include <stdint.h>
#include <gui/common/Kit.hpp>
// Sense & temperatures: remote sense control and readings, T1-T4 levels, fan.
class ScreenPowerPathView : public ScreenPowerPathViewBase {
public:
    ScreenPowerPathView();
    virtual ~ScreenPowerPathView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void togglePermit();
    virtual void toggleSense();
    virtual void refreshTelemetry();
protected:
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    bool remoteOn;
    void refresh();
    void notify(const char* text);
    void layout();
    void onClick(const kit::Btn& b);
    kit::Header header;
    kit::Card senseCard, tempCard, fanCard;
    kit::Panel segment;
    kit::Btn modeLocal, modeRemote, permitBtn, refreshBtn, fanIcon;
    kit::Btn localBox, loadBox;
    kit::Rows senseRows;
    kit::Label footnote, tempValue[4], tempName[4], fanTitle, fanSub;
    kit::Level tempLevel[4];
    kit::Value fanValue;
    touchgfx::Callback<ScreenPowerPathView, const kit::Btn&> clicked;
};
#endif
