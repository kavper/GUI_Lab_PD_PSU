#include <gui/common/UiTheme.hpp>
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
void ScreenAppearanceView::allOff() { psu_app_shutdown(); }

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
