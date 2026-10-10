#include <gui/common/UiTheme.hpp>
#include <images/BitmapDatabase.hpp>
#include <gui/screensettings_screen/ScreenSettingsView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/FrontendApplication.hpp>

ScreenSettingsView::ScreenSettingsView()
{

}

void ScreenSettingsView::setupScreen()
{
    ScreenSettingsViewBase::setupScreen();
    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::SETTINGS);
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,"Configure, monitor and automate",lab_muted());

    setupTheme();
}

void ScreenSettingsView::tearDownScreen()
{
    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::OTHER);
    ScreenSettingsViewBase::tearDownScreen();
}

extern "C" {
#include "psu_app.h"
}
void ScreenSettingsView::allOff() { psu_app_power_shutdown(); }

void ScreenSettingsView::setupTheme()
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
    theme.button(PresetManagerButton,ui::NORMAL);
    theme.image(Icon0,ui::ICON,&PresetManagerButton);
    theme.text(Title0);
    theme.text(Desc0);
    theme.button(SequencerButton,ui::NORMAL);
    theme.image(Icon1,ui::ICON,&SequencerButton);
    theme.text(Title1);
    theme.text(Desc1);
    theme.button(ChargerButton,ui::NORMAL);
    theme.image(Icon2,ui::ICON,&ChargerButton);
    theme.text(Title2);
    theme.text(Desc2);
    theme.button(PackButton,ui::NORMAL);
    theme.image(Icon3,ui::ICON,&PackButton);
    theme.text(Title3);
    theme.text(Desc3);
    theme.button(UsbButton,ui::NORMAL);
    theme.image(Icon4,ui::ICON,&UsbButton);
    theme.text(Title4);
    theme.text(Desc4);
    theme.button(DiagnosticsButton,ui::NORMAL);
    theme.image(Icon5,ui::ICON,&DiagnosticsButton);
    theme.text(Title5);
    theme.text(Desc5);
    theme.button(SystemButton,ui::NORMAL);
    theme.image(Icon6,ui::ICON,&SystemButton);
    theme.text(Title6);
    theme.text(Desc6);
    theme.button(AppearanceButton,ui::NORMAL);
    theme.image(AppearanceIcon,ui::ICON,&AppearanceButton);
    theme.text(AppearanceTitle);
    theme.text(AppearanceDesc);
    theme.apply();
}
