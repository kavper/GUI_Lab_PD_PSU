#include <gui/screenservice_screen/ScreenServiceView.hpp>
#include <gui/common/KitPage.hpp>
#include <gui/common/UiTheme.hpp>
extern "C" {
#include "psu_app.h"
}

namespace {
// Console history survives page changes.
const unsigned LOG_LINES = 14;
char logLines[LOG_LINES][48];
unsigned logCount = 0;
char lastTx[48], lastRx[48];
void logAppend(const char* dir, const char* text)
{
    if (logCount == LOG_LINES) { memmove(logLines[0], logLines[1], sizeof(logLines[0]) * (LOG_LINES - 1)); --logCount; }
    const uint32_t s = psu_app_now() / 1000;
    snprintf(logLines[logCount++], sizeof(logLines[0]), "%02lu:%02lu %s %.36s", (unsigned long)(s / 60 % 60), (unsigned long)(s % 60), dir, text);
}
}

ScreenServiceView::ScreenServiceView() : divider(0), clicked(this, &ScreenServiceView::onClick) { notice[0] = 0; }

void ScreenServiceView::setupScreen()
{
    ScreenServiceViewBase::setupScreen();
    layout();
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this); theme.box(LabBackground, ui::BACKGROUND); theme.apply();
    refresh();
}

void ScreenServiceView::layout()
{
    KIT_PAGE_BEGIN(FrontendApplication::SERVICE);
    consoleCard.setup(12, 58, 516, 410, "CONTROLLER CONSOLE", "G4 \xC2\xB7 binary + ASCII");
    lockCard.setup(538, 58, 250, 124, "");
    add(consoleCard); add(lockCard);
    console.setup(25, 84, 490, LOG_LINES * 24, 24);
    console.setMonoPlain(true);
    consoleNote.setup(kit::SUB, kit::LEFT, 25, 444, 490, 16, ui::MUTED);
    add(console); add(consoleNote);
    lockIcon.setup(638, 66, 50, 40, "", kit::Btn::GHOST);
    lockIcon.setBase(ui::SURFACE); lockIcon.setIcon(kit::ICON_LOCK); lockIcon.setIconSize(24); lockIcon.setTouchable(false);
    add(lockIcon);
    lockState.setup(kit::HEAD, kit::CENTER, 538, 108, 250, 30, ui::CAUTION);
    lockSub.setup(kit::SUB, kit::CENTER, 538, 142, 250, 16, ui::MUTED);
    lockSub.set("Service mode");
    add(lockState); add(lockSub);
    toggleBtn.setup(538, 192, 250, 50, "UNLOCK SERVICE", kit::Btn::OUTLINE, kit::BODYM);
    statusBtn.setup(538, 252, 250, 46, "SEND STATUS", kit::Btn::NORMAL, kit::BODYM);
    helpBtn.setup(538, 308, 250, 46, "HELP", kit::Btn::NORMAL, kit::BODYM);
    clearBtn.setup(538, 418, 250, 50, "CLEAR LOG", kit::Btn::NORMAL, kit::BODYM);
    kit::Btn* b[] = { &toggleBtn, &statusBtn, &helpBtn, &clearBtn };
    for (int i = 0; i < 4; ++i) { b[i]->setBase(ui::BACKGROUND); b[i]->tag = i; b[i]->setAction(clicked); add(*b[i]); }
    KIT_PAGE_HEADER(FrontendApplication::SERVICE);
}

void ScreenServiceView::onClick(const kit::Btn& b)
{
    switch (b.tag) {
    case 0: serviceToggle(); break;
    case 1: serviceStatus(); break;
    case 2: serviceHelp(); break;
    case 3: logCount = 0; refresh(); break;
    }
}

void ScreenServiceView::handleTickEvent()
{
    if (kit::app()->isScreenTransitionActive()) return;
    if (++divider < 8) return;
    divider = 0;
    refresh();
}

void ScreenServiceView::refresh()
{
    PsuSnapshot snap;
    psu_app_ensure();
    psu_snapshot(&snap);
    if (snap.g4_last_tx[0] && strcmp(snap.g4_last_tx, lastTx)) { snprintf(lastTx, sizeof(lastTx), "%s", snap.g4_last_tx); logAppend("\xE2\x86\x92", lastTx); }
    if (snap.g4_last_rx[0] && strcmp(snap.g4_last_rx, lastRx)) { snprintf(lastRx, sizeof(lastRx), "%s", snap.g4_last_rx); logAppend("\xE2\x86\x90", lastRx); }
    static char text[LOG_LINES * 50];
    text[0] = 0;
    for (unsigned i = 0; i < logCount; ++i) { size_t n = strlen(text); snprintf(text + n, sizeof(text) - n, "%s\n", logLines[i]); }
    if (!logCount) strcpy(text, "No controller traffic yet\n");
    console.set(text, true);
    const bool open = snap.service_mode != 0;
    consoleNote.set(open ? "Service commands are sent to the controllers." : "Read-only while locked. Unlock to send service commands.");
    lockState.set(open ? "OPEN" : "LOCKED", open ? ui::POSITIVE : ui::CAUTION);
    lockIcon.setInk(open ? ui::POSITIVE : ui::CAUTION);
    lockCard.setBorder(open ? ui::POSITIVE : ui::BORDER);
    toggleBtn.setLabel(open ? "LOCK SERVICE" : "UNLOCK SERVICE");
    statusBtn.setEnabled(open);
    helpBtn.setEnabled(open);
    header.setSub(notice[0] ? notice : "Service commands and controller replies", ui::MUTED);
}

void ScreenServiceView::result(bool accepted)
{
    snprintf(notice, sizeof(notice), "%s", accepted ? "Request accepted \xC2\xB7 check the reply in the console" : "Request blocked \xC2\xB7 check connection or service state");
    refresh();
}
void ScreenServiceView::serviceToggle() { result(psu_app_service(!psu_g4()->service_mode) != 0); }
void ScreenServiceView::serviceHelp() { result(psu_app_service_cmd("HELP") != 0); }
void ScreenServiceView::serviceStatus() { result(psu_app_service_cmd("STATUS") != 0); }
void ScreenServiceView::allOff() { psu_app_power_shutdown(); }
