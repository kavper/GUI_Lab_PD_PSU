#include <gui/screensettings_screen/ScreenSettingsView.hpp>
#include <gui/common/LabText.hpp>

ScreenSettingsView::ScreenSettingsView()
{

}

void ScreenSettingsView::setupScreen()
{
    ScreenSettingsViewBase::setupScreen();
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,"Configure, monitor and automate",lab_muted());
}

void ScreenSettingsView::tearDownScreen()
{
    ScreenSettingsViewBase::tearDownScreen();
}

extern "C" {
#include "psu_app.h"
}
void ScreenSettingsView::allOff() { psu_app_shutdown(); }
