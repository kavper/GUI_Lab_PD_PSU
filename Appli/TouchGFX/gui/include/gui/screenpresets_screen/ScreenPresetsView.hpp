#pragma once
#include <gui_generated/screenpresets_screen/ScreenPresetsViewBase.hpp>
#include <gui/screenpresets_screen/ScreenPresetsPresenter.hpp>
extern "C" {
#include "psu_edit.h"
}
class ScreenPresetsView : public ScreenPresetsViewBase {
public:
 ScreenPresetsView();
 virtual void setupScreen();
 virtual void tearDownScreen() {ScreenPresetsViewBase::tearDownScreen();}
 virtual void allOff();
 virtual void loadPreset();
 virtual void editVoltage();
 virtual void editCurrent();
 virtual void selectPreset1();
 virtual void selectPreset2();
 virtual void selectPreset3();
 virtual void presetKey0();
 virtual void presetKey1();
 virtual void presetKey2();
 virtual void presetKey3();
 virtual void presetKey4();
 virtual void presetKey5();
 virtual void presetKey6();
 virtual void presetKey7();
 virtual void presetKey8();
 virtual void presetKey9();
 virtual void presetKeyDot();
 virtual void presetKeyClr();
 virtual void presetKeyDel();
 virtual void presetKeyApply();

private:
 unsigned selected, field;
 PsuEditor editor;
 void select(unsigned index);
 void refresh();
 void key(char c);
};
