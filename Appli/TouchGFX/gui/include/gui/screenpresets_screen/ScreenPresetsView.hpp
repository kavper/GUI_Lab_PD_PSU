#ifndef SCREENPRESETSVIEW_HPP
#define SCREENPRESETSVIEW_HPP

#include <gui_generated/screenpresets_screen/ScreenPresetsViewBase.hpp>
#include <gui/screenpresets_screen/ScreenPresetsPresenter.hpp>
#include <stdint.h>
#include <gui/common/LabSwipe.hpp>

class ScreenPresetsView : public ScreenPresetsViewBase
{
public:
    ScreenPresetsView();
    virtual ~ScreenPresetsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
    virtual void handleTickEvent();
    virtual void loadPreset();
    virtual void savePreset();
    virtual void duplicatePreset();
    virtual void resetPresets();
    virtual void selectPreset1();
    virtual void selectPreset2();
    virtual void selectPreset3();
    virtual void selectPreset4();
    virtual void selectPreset5();
    virtual void selectPreset6();
    virtual void selectPreset7();
    virtual void selectPreset8();

protected:
    LabSwipe swipe;
    uint8_t divider;
    uint8_t selected;
    void refresh();
    void selectSlot(uint8_t index);
};

#endif
