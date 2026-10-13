#include <gui/common/UiTheme.hpp>
#include <gui/common/UiSound.hpp>
#include <images/BitmapDatabase.hpp>
#include <gui/screenappearance_screen/ScreenAppearanceView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/FrontendApplication.hpp>

ScreenAppearanceView::ScreenAppearanceView()
{

}

void ScreenAppearanceView::setupScreen()
{
    ScreenAppearanceViewBase::setupScreen();
    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::OTHER);
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,"Configure, monitor and automate",lab_muted());

    setupTheme();
    refreshThemeControls();
}

void ScreenAppearanceView::tearDownScreen()
{
    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::OTHER);
    ScreenAppearanceViewBase::tearDownScreen();
}

extern "C" {
#include "psu_app.h"
}
void ScreenAppearanceView::allOff() { psu_app_power_shutdown(); }

void ScreenAppearanceView::setupTheme()
{
    ui::ThemeScreen& theme=ui::ThemeScreen::get();
    theme.begin(*this);
    theme.box(LabBackground,ui::BACKGROUND);
    theme.box(LabHeader,ui::SURFACE);
    theme.box(ThemeHeaderDivider,ui::BORDER);
    theme.text(PageTitle);
    theme.text(PageFeedback);
    theme.button(AllOffButton,ui::DANGER);
    theme.button(BackButton,ui::NORMAL);
    theme.panel(ThemePanel);
    theme.panel(AccentPanel);
    theme.panel(SoundPanel);
    theme.text(SoundHeading,ui::ACCENT);
    theme.text(SoundStatus,ui::MUTED);
    theme.button(SoundMuteButton);
    theme.button(SoundDownButton);
    theme.button(SoundUpButton);
    theme.button(SoundTestButton,ui::PRIMARY);
    theme.button(SoundToneButton);
    theme.button(SoundFeedbackButton);
    theme.text(ThemeDetail);
    theme.text(ThemeHeading);
    theme.text(AccentHeading);
    theme.button(LightThemeButton,ui::NORMAL);
    theme.button(DarkThemeButton,ui::NORMAL);
    theme.button(AccentBlueButton,ui::SWATCH_BLUE);
    theme.button(AccentTealButton,ui::SWATCH_TEAL);
    theme.button(AccentVioletButton,ui::SWATCH_VIOLET);
    theme.button(AccentAmberButton,ui::SWATCH_AMBER);
    theme.text(ThemeHint);
    theme.apply();
}

void ScreenAppearanceView::refreshThemeControls()
{
    const touchgfx::Bitmap normal(BITMAP_UX_ACTION_REL_88X48_ID),selected(BITMAP_UX_ACTION_SEL_88X48_ID);
    LightThemeButton.setBitmaps(ui::Theme::dark()?normal:selected,selected);
    DarkThemeButton.setBitmaps(ui::Theme::dark()?selected:normal,selected);
    SoundMuteButton.setBitmaps(ui::Sound::enabled()?selected:normal,selected);
    char sound[100];
    const unsigned hz[]={4000,2000,1000};
    const char* response=ui::Sound::feedback()==3?"Touch + controls":ui::Sound::feedback()==1?"Touch only":"Controls only";
    snprintf(sound,sizeof(sound),"%s / Volume %u%% / %u kHz\n%s",ui::Sound::enabled()?"ON":"MUTED",ui::Sound::volume(),hz[ui::Sound::tone()]/1000,response);
    lab_show(SoundStatus,SoundStatusBuffer,SOUNDSTATUS_SIZE,sound,lab_muted());
    char feedback[100];snprintf(feedback,sizeof(feedback),"%s theme / %s accent",ui::Theme::dark()?"Dark":"Light",ui::Theme::accentName());
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,feedback,lab_muted());
    ui::ThemeScreen::get().apply();
}
void ScreenAppearanceView::setLightTheme(){ui::Theme::setDark(false);refreshThemeControls();}
void ScreenAppearanceView::setDarkTheme(){ui::Theme::setDark(true);refreshThemeControls();}
void ScreenAppearanceView::setBlueAccent(){ui::Theme::setAccent(0);refreshThemeControls();}
void ScreenAppearanceView::setTealAccent(){ui::Theme::setAccent(1);refreshThemeControls();}
void ScreenAppearanceView::setVioletAccent(){ui::Theme::setAccent(2);refreshThemeControls();}
void ScreenAppearanceView::setAmberAccent(){ui::Theme::setAccent(3);refreshThemeControls();}

void ScreenAppearanceView::toggleSound(){ui::Sound::toggle();refreshThemeControls();}
void ScreenAppearanceView::soundDown(){ui::Sound::setVolume(ui::Sound::volume()>10?ui::Sound::volume()-10:0);refreshThemeControls();}
void ScreenAppearanceView::soundUp(){ui::Sound::setVolume(ui::Sound::volume()+10);refreshThemeControls();}
void ScreenAppearanceView::testSound(){ui::Sound::test();refreshThemeControls();}
void ScreenAppearanceView::cycleTone(){ui::Sound::cycleTone();refreshThemeControls();ui::Sound::test();}
void ScreenAppearanceView::cycleFeedback(){ui::Sound::cycleFeedback();refreshThemeControls();}
