#pragma once
#include <gui_generated/screenpresets_screen/ScreenPresetsViewBase.hpp>
#include <gui/screenpresets_screen/ScreenPresetsPresenter.hpp>
#include <gui/common/Kit.hpp>
#include <stdio.h>
extern "C" {
#include "psu_edit.h"
}
// Presets: three preset rows (tap a value to edit, APPLY loads it) and an
// editor card with a keypad.
class ScreenPresetsView : public ScreenPresetsViewBase {
public:
    ScreenPresetsView();
    virtual void setupScreen();
    virtual void handleTickEvent();
    virtual void tearDownScreen() { ScreenPresetsViewBase::tearDownScreen(); }
    virtual void allOff();
private:
    enum { KEYS = 12 };
    bool storageWatch;
    const char* lastStorageStatus;
    unsigned selected, field;
    PsuEditor editor;
    char notice[80];
    kit::Header header;
    kit::Panel row[3], segment;
    kit::Label name[3], nameSub[3], maxCaption[3], footnote, editValue, editRange;
    kit::Value maxValue[3];
    kit::Btn volt[3], amp[3], apply[3], fieldBtn[2], keys[KEYS], save;
    kit::Card editCard;
    touchgfx::Callback<ScreenPresetsView, const kit::Btn&> clicked;
    void layout();
    void onClick(const kit::Btn& b);
    void select(unsigned index, unsigned f);
    void refresh();
    void setNotice(const char* t) { snprintf(notice, sizeof(notice), "%s", t); }
};
