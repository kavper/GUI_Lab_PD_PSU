#pragma once
#include <gui_generated/screenextcharger_screen/ScreenExtChargerViewBase.hpp>
#include <gui/screenextcharger_screen/ScreenExtChargerPresenter.hpp>
#include <gui/common/SessionChart.hpp>
extern "C" {
#include "psu_edit.h"
}
class ScreenExtChargerView: public ScreenExtChargerViewBase {
public:
 ScreenExtChargerView():page(0),field(1),divider(0),polarity(false){psu_editor_clear(&editor);}
 virtual void setupScreen();
 virtual void tearDownScreen(){ScreenExtChargerViewBase::tearDownScreen();}
 virtual void handleTickEvent(){if(++divider>=15){divider=0;refresh();}}
 virtual void allOff();
 virtual void showSetup();
 virtual void showSession();
 virtual void showOnboard();
 virtual void confirmPolarity();
 virtual void startCharge();
 virtual void stopCharge();
 virtual void chem0();
 virtual void chem1();
 virtual void chem2();
 virtual void chem3();
 virtual void chem4();
 virtual void field0();
 virtual void field1();
 virtual void field2();
 virtual void field3();
 virtual void chargeKey0();
 virtual void chargeKey1();
 virtual void chargeKey2();
 virtual void chargeKey3();
 virtual void chargeKey4();
 virtual void chargeKey5();
 virtual void chargeKey6();
 virtual void chargeKey7();
 virtual void chargeKey8();
 virtual void chargeKey9();
 virtual void chargeKeyDot();
 virtual void chargeKeyClr();
 virtual void chargeKeyDel();
 virtual void chargeKeyApply();

private:
 unsigned page,field,divider;bool polarity;PsuEditor editor;SessionChart chart;
 void refresh();void selectField(unsigned i);void chooseChem(unsigned i);void key(char k);
};
