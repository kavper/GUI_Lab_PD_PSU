#ifndef SCREENPRESETSVIEW_HPP
#define SCREENPRESETSVIEW_HPP

#include <gui_generated/screenpresets_screen/ScreenPresetsViewBase.hpp>
#include <gui/screenpresets_screen/ScreenPresetsPresenter.hpp>
#include <stdint.h>

class ScreenPresetsView : public ScreenPresetsViewBase
{
public:
    ScreenPresetsView();
    virtual ~ScreenPresetsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void loadPreset();
    virtual void savePreset();
    virtual void duplicatePreset();
    virtual void resetPresets();
protected:
    uint8_t divider;
    void refresh();
};

#endif
