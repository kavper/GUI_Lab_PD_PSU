#ifndef SCREENSETTINGSVIEW_HPP
#define SCREENSETTINGSVIEW_HPP

#include <gui_generated/screensettings_screen/ScreenSettingsViewBase.hpp>
#include <gui/screensettings_screen/ScreenSettingsPresenter.hpp>
#include <gui/common/Kit.hpp>

// Quick menu sheet: opened by pulling down from the top edge of any page.
class ScreenSettingsView : public ScreenSettingsViewBase
{
public:
    ScreenSettingsView();
    virtual ~ScreenSettingsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
private:
    static const int TILE_COUNT = 14;
    kit::Fill dim;
    kit::Panel sheet;
    kit::Fill handle;
    kit::Chip chipOut, chipV, chipA, chipW;
    kit::Btn power, closeStrip;
    kit::Label monitorCaption, toolsCaption, hint;
    kit::Btn tiles[TILE_COUNT];
    uint16_t divider, armTicks;
    touchgfx::Callback<ScreenSettingsView, const kit::Btn&> tileCallback, powerCallback, closeCallback;
    void onTile(const kit::Btn& b);
    void onPower(const kit::Btn& b);
    void onClose(const kit::Btn& b);
    void refresh();
};

#endif // SCREENSETTINGSVIEW_HPP
