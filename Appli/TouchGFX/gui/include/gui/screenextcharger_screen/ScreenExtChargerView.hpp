#pragma once
#include <gui_generated/screenextcharger_screen/ScreenExtChargerViewBase.hpp>
#include <gui/screenextcharger_screen/ScreenExtChargerPresenter.hpp>
#include <gui/common/Kit.hpp>
extern "C" {
#include "psu_edit.h"
#include "psu_app.h"
}
// Cell charger: a three step setup (chemistry, pack, safety) on the left and
// the live session (phase, V / A, counters, profile chart) on the right. While
// a value is edited the session card turns into a keypad.
class ScreenExtChargerView: public ScreenExtChargerViewBase {
public:
    ScreenExtChargerView();
    virtual void setupScreen();
    virtual void tearDownScreen() { ScreenExtChargerViewBase::tearDownScreen(); }
    virtual void handleTickEvent();
    virtual void allOff();
    // Designer interactions (widgets are hidden; kept for the generated base).
    virtual void showSetup() {}
    virtual void showSession() {}
    virtual void showOnboard() {}
    virtual void confirmPolarity();
    virtual void startCharge();
    virtual void stopCharge();
    virtual void chem0() {} virtual void chem1() {} virtual void chem2() {} virtual void chem3() {} virtual void chem4() {}
    virtual void field0() {} virtual void field1() {} virtual void field2() {} virtual void field3() {} virtual void field4() {}
    virtual void chargeKey0() {} virtual void chargeKey1() {} virtual void chargeKey2() {} virtual void chargeKey3() {}
    virtual void chargeKey4() {} virtual void chargeKey5() {} virtual void chargeKey6() {} virtual void chargeKey7() {}
    virtual void chargeKey8() {} virtual void chargeKey9() {} virtual void chargeKeyDot() {} virtual void chargeKeyClr() {}
    virtual void chargeKeyDel() {} virtual void chargeKeyApply() {}

private:
    enum { CHEMS = 6, KEYS = 14 };
    enum Field { NONE = -1, TARGET = 0, CURRENT = 1, STOP = 2 };
    int field;
    unsigned divider;
    bool polarity;
    PsuEditor editor;

    kit::Header header;
    kit::Panel setupCard, sessionCard, cellsBox;
    kit::Btn step[3]; kit::Label stepCaption[3], cellsValue, cellsSub, note;
    kit::Btn chem[CHEMS], cellsMinus, cellsPlus, target, current, stop, polarityBtn;
    kit::Btn phase[3], start, stopBtn;
    kit::Label plotEmpty, liveV, liveA, statCaption[3], statValue[3], footnote;
    kit::Plot plot;
    kit::Label editCaption, editValue, editHint;
    kit::Btn keys[KEYS];
    touchgfx::Callback<ScreenExtChargerView, const kit::Btn&> clicked;

    void layout();
    void setupTheme();
    void refresh();
    void onClick(const kit::Btn& b);
    void edit(int f);
    void key(char k);
    void apply();
    void setCells(int delta);
    void showEditor(bool on);
};
