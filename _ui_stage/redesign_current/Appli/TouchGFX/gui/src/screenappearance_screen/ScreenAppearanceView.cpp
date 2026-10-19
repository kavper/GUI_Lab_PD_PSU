#include <gui/screenappearance_screen/ScreenAppearanceView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenAppearanceView::ScreenAppearanceView()
{
}

void ScreenAppearanceView::setupScreen()
{
    ScreenAppearanceViewBase::setupScreen();
    refresh();
}

void ScreenAppearanceView::tearDownScreen()
{
    ScreenAppearanceViewBase::tearDownScreen();
}

void ScreenAppearanceView::refresh()
{
    PsuAppearanceFace face;
    psu_app_ensure();
    psu_appearance_face(&face);
    lab_show(AppDisplay, AppDisplayBuffer, APPDISPLAY_SIZE, face.display, lab_cyan());
    lab_show(AppType, AppTypeBuffer, APPTYPE_SIZE, face.typeface, lab_text());
    lab_show(AppSound, AppSoundBuffer, APPSOUND_SIZE, face.sound, lab_muted());
    lab_show(AppStored, AppStoredBuffer, APPSTORED_SIZE, face.stored, lab_amber());
    lab_show(AppNote, AppNoteBuffer, APPNOTE_SIZE, face.note, lab_text());
}

void ScreenAppearanceView::handleClickEvent(const touchgfx::ClickEvent& event)
{
    swipe.onClick(event, false);
    ScreenAppearanceViewBase::handleClickEvent(event);
}

void ScreenAppearanceView::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    if (swipe.onGesture(event))
    {
        application().gotoScreenSettingsScreenWipeTransitionEast();
        return;
    }
    ScreenAppearanceViewBase::handleGestureEvent(event);
}
