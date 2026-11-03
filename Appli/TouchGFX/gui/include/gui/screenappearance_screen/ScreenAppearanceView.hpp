#ifndef SCREENAPPEARANCEVIEW_HPP
#define SCREENAPPEARANCEVIEW_HPP
#include <gui_generated/screenappearance_screen/ScreenAppearanceViewBase.hpp>
#include <gui/screenappearance_screen/ScreenAppearancePresenter.hpp>
#include <gui/common/Kit.hpp>
// Appearance: theme tiles, accent and backlight brightness, sound feedback.
class ScreenAppearanceView : public ScreenAppearanceViewBase
{
public:
    ScreenAppearanceView();
    virtual ~ScreenAppearanceView() {}
    virtual void setupScreen();
    virtual void tearDownScreen() { ScreenAppearanceViewBase::tearDownScreen(); }
    virtual void handleTickEvent();
    virtual void allOff();
protected:
    const char* lastStorageStatus;
    void refresh();
    void layout();
    void onTheme(const kit::ThemeSwatch& s);
    void onSlider(const kit::Slider& s, bool final);
    void onClick(const kit::Btn& b);
    kit::Header header;
    kit::Card themeCard, accentCard, soundCard;
    kit::ThemeSwatch swatch[4];
    kit::Btn accent[4], soundSwitch, toneBtn, feedbackBtn, testBtn;
    kit::Label accentNote, brightnessCaption, brightnessValue, keyLabel, volumeLabel, volumeValue, toneLabel, footnote;
    kit::Slider brightness, volume;
    touchgfx::Callback<ScreenAppearanceView, const kit::ThemeSwatch&> themeCallback;
    touchgfx::Callback<ScreenAppearanceView, const kit::Slider&, bool> sliderCallback;
    touchgfx::Callback<ScreenAppearanceView, const kit::Btn&> clicked;
};
#endif // SCREENAPPEARANCEVIEW_HPP
