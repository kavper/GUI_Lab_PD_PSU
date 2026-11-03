#include <gui/screenappearance_screen/ScreenAppearanceView.hpp>
#include <gui/common/KitPage.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/UiSound.hpp>
#include <gui/common/SettingsPersistence.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenAppearanceView::ScreenAppearanceView()
    : lastStorageStatus(0), themeCallback(this, &ScreenAppearanceView::onTheme),
      sliderCallback(this, &ScreenAppearanceView::onSlider), clicked(this, &ScreenAppearanceView::onClick) {}

void ScreenAppearanceView::setupScreen()
{
    ScreenAppearanceViewBase::setupScreen();
    layout();
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this); theme.box(LabBackground, ui::BACKGROUND); theme.apply();
    refresh();
}

void ScreenAppearanceView::layout()
{
    KIT_PAGE_BEGIN(FrontendApplication::APPEARANCE);
    themeCard.setup(12, 58, 776, 158, "THEME");
    accentCard.setup(12, 226, 384, 214, "ACCENT");
    soundCard.setup(406, 226, 382, 214, "SOUND & FEEDBACK");
    add(themeCard); add(accentCard); add(soundCard);
    for (unsigned i = 0; i < 4; ++i) {
        swatch[i].setup(24 + i * 190, 84, 182, 120, i);
        swatch[i].setAction(themeCallback);
        add(swatch[i]);
    }
    static const ui::SurfaceStyle unused[] = { ui::SWATCH_BLUE, ui::SWATCH_TEAL, ui::SWATCH_VIOLET, ui::SWATCH_AMBER };
    (void)unused;
    for (int i = 0; i < 4; ++i) {
        accent[i].setup(24 + i * 52, 254, 44, 44, "", kit::Btn::SWATCH);
        accent[i].setBase(ui::SURFACE); accent[i].tag = i; accent[i].setAction(clicked);
        add(accent[i]);
    }
    accentNote.setup(kit::SUB, kit::LEFT, 236, 262, 150, 32, ui::MUTED);
    add(accentNote);
    brightnessCaption.setup(kit::CAP, kit::LEFT, 25, 322, 200, 14, ui::MUTED);
    brightnessCaption.set("BACKLIGHT BRIGHTNESS");
    brightness.setup(24, 344, 290, 44);
    brightness.setAction(sliderCallback);
    brightnessValue.setup(kit::M16, kit::RIGHT, 316, 354, 68, 24, ui::TEXT);
    add(brightnessCaption); add(brightness); add(brightnessValue);
    kit::addNoSwipe(touchgfx::Rect(12, 336, 384, 60));

    keyLabel.setup(kit::BODY, kit::LEFT, 419, 262, 200, 20, ui::TEXT);
    keyLabel.set("Key sounds");
    soundSwitch.setup(700, 252, 76, 36, "OFF", kit::Btn::NORMAL, kit::BODYM);
    soundSwitch.setBase(ui::SURFACE); soundSwitch.tag = 10; soundSwitch.setAction(clicked);
    volumeLabel.setup(kit::BODY, kit::LEFT, 419, 312, 80, 20, ui::TEXT);
    volumeLabel.set("Volume");
    volume.setup(490, 300, 220, 44);
    volume.setAction(sliderCallback);
    volumeValue.setup(kit::M14, kit::RIGHT, 712, 312, 64, 20, ui::TEXT);
    toneLabel.setup(kit::SUB, kit::LEFT, 419, 360, 360, 16, ui::MUTED);
    toneBtn.setup(418, 384, 112, 44, "TONE", kit::Btn::NORMAL, kit::BODYM);
    feedbackBtn.setup(536, 384, 118, 44, "FEEDBACK", kit::Btn::NORMAL, kit::BODYM);
    testBtn.setup(660, 384, 116, 44, "TEST", kit::Btn::PRIMARY, kit::BODYM);
    toneBtn.tag = 11; feedbackBtn.tag = 12; testBtn.tag = 13;
    kit::Btn* b[] = { &toneBtn, &feedbackBtn, &testBtn };
    for (kit::Btn* x : b) { x->setBase(ui::SURFACE); x->setAction(clicked); }
    add(keyLabel); add(soundSwitch); add(volumeLabel); add(volume); add(volumeValue); add(toneLabel);
    add(toneBtn); add(feedbackBtn); add(testBtn);
    kit::addNoSwipe(touchgfx::Rect(406, 296, 382, 52));
    footnote.setup(kit::SUB, kit::LEFT, 14, 450, 770, 16, ui::MUTED);
    add(footnote);
    KIT_PAGE_HEADER(FrontendApplication::APPEARANCE);
}

void ScreenAppearanceView::refresh()
{
    const unsigned current = ui::Theme::themeIndex();
    for (unsigned i = 0; i < 4; ++i) swatch[i].setSelected(current == i);
    const bool accents = ui::Theme::accentSelectable();
    for (unsigned i = 0; i < 4; ++i) {
        accent[i].setVisible(accents);
        accent[i].setSelected(accents && ui::Theme::accentIndex() == i);
        accent[i].invalidate();
    }
    accentNote.set(accents ? ui::Theme::accentName() : "Accent is fixed\nby this theme");
    PsuAppearance a; psu_appearance_get(&a);
    brightness.set((a.brightness - PSU_BRIGHTNESS_MIN) * 1000 / (100 - PSU_BRIGHTNESS_MIN));
    char t[128];
    snprintf(t, sizeof(t), "%u %%", a.brightness); brightnessValue.set(t);
    const bool on = ui::Sound::enabled();
    soundSwitch.setLabel(on ? "ON" : "OFF");
    soundSwitch.setStyle(on ? kit::Btn::POSITIVE_FILL : kit::Btn::NORMAL);
    volume.set(ui::Sound::volume() * 10);
    snprintf(t, sizeof(t), "%u %%", ui::Sound::volume()); volumeValue.set(t);
    static const unsigned hz[] = { 4000, 2000, 1000 };
    const char* response = ui::Sound::feedback() == 3 ? "touch + controls" : ui::Sound::feedback() == 1 ? "touch only" : "controls only";
    snprintf(t, sizeof(t), "Tone %u kHz \xC2\xB7 %s", hz[ui::Sound::tone() % 3] / 1000, response);
    toneLabel.set(t);
    snprintf(t, sizeof(t), "Theme and sound settings are saved automatically \xC2\xB7 %s", ui::settingsPersistenceStatus());
    footnote.set(t);
    snprintf(t, sizeof(t), "%s%s%s", ui::Theme::themeName(current), accents ? " \xC2\xB7 " : "", accents ? ui::Theme::accentName() : "");
    header.setSub(t, ui::MUTED);
}

void ScreenAppearanceView::handleTickEvent()
{
    ScreenAppearanceViewBase::handleTickEvent();
    const char* status = ui::settingsPersistenceStatus();
    if (status != lastStorageStatus) { lastStorageStatus = status; refresh(); }
}

void ScreenAppearanceView::onTheme(const kit::ThemeSwatch& s)
{
    ui::Theme::setTheme(s.themeIndex());
    getRootContainer().invalidate();
    refresh();
}

void ScreenAppearanceView::onSlider(const kit::Slider& s, bool final)
{
    char t[16];
    if (&s == &brightness) {
        PsuAppearance a; psu_appearance_get(&a);
        a.brightness = (uint8_t)(PSU_BRIGHTNESS_MIN + (s.get() * (100 - PSU_BRIGHTNESS_MIN) + 500) / 1000);
        psu_appearance_set(&a); // applied by the GUI task on the next tick, then saved
        snprintf(t, sizeof(t), "%u %%", a.brightness); brightnessValue.set(t);
    } else {
        ui::Sound::setVolume((unsigned)((s.get() + 5) / 10));
        snprintf(t, sizeof(t), "%u %%", ui::Sound::volume()); volumeValue.set(t);
        if (final) ui::Sound::test();
    }
    if (final) refresh();
}

void ScreenAppearanceView::onClick(const kit::Btn& b)
{
    if (b.tag < 4) { ui::Theme::setAccent(b.tag); getRootContainer().invalidate(); }
    else if (b.tag == 10) ui::Sound::toggle();
    else if (b.tag == 11) { ui::Sound::cycleTone(); ui::Sound::test(); }
    else if (b.tag == 12) ui::Sound::cycleFeedback();
    else if (b.tag == 13) ui::Sound::test();
    refresh();
}

void ScreenAppearanceView::allOff() { psu_app_power_shutdown(); }
