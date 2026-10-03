#include <gui/screenservice_screen/ScreenServiceView.hpp>
#include <gui/common/LabText.hpp>
#include <texts/TextKeysAndLanguages.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenServiceView::ScreenServiceView()
    : divider(0)
{
}

void ScreenServiceView::setupScreen()
{
    ScreenServiceViewBase::setupScreen();
    refresh();
}

void ScreenServiceView::tearDownScreen()
{
    ScreenServiceViewBase::tearDownScreen();
}

void ScreenServiceView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenServiceView::refresh()
{
    PsuSnapshot snap;
    psu_app_ensure();
    psu_snapshot(&snap);
    lab_enable(HelpButton, snap.service_mode != 0);
    lab_enable(RefreshButton, snap.service_mode != 0);
    ServiceButton.setLabelText(touchgfx::TypedText(snap.service_mode ? T_TXT_SERVICE_DISABLE : T_TXT_SERVICE_ENABLE));
    ServiceButton.invalidate();
    lab_show(SvcMode, SvcModeBuffer, SVCMODE_SIZE, snap.service_mode ? "OPEN" : "LOCKED",
             snap.service_mode ? lab_green() : lab_amber());
    lab_show(SvcTx, SvcTxBuffer, SVCTX_SIZE, snap.g4_last_tx[0] ? snap.g4_last_tx : "--", lab_cyan());
    lab_show(SvcRx, SvcRxBuffer, SVCRX_SIZE, snap.g4_last_rx[0] ? snap.g4_last_rx : "--", lab_text());
}

void ScreenServiceView::serviceToggle()
{
    const bool accepted = psu_app_service(!psu_g4()->service_mode) != 0;
    lab_show(PageFeedback, PageFeedbackBuffer, PAGEFEEDBACK_SIZE,
             accepted ? "Request accepted. Check the reported state below." : "Request blocked. Check connection, protection or service state.",
             accepted ? lab_cyan() : lab_amber());
    refresh();
}
void ScreenServiceView::serviceHelp()
{
    const bool accepted = psu_app_service_cmd("HELP") != 0;
    lab_show(PageFeedback, PageFeedbackBuffer, PAGEFEEDBACK_SIZE,
             accepted ? "Request accepted. Check the reported state below." : "Request blocked. Check connection, protection or service state.",
             accepted ? lab_cyan() : lab_amber());
    refresh();
}
void ScreenServiceView::serviceStatus()
{
    const bool accepted = psu_app_service_cmd("STATUS") != 0;
    lab_show(PageFeedback, PageFeedbackBuffer, PAGEFEEDBACK_SIZE,
             accepted ? "Request accepted. Check the reported state below." : "Request blocked. Check connection, protection or service state.",
             accepted ? lab_cyan() : lab_amber());
    refresh();
}

extern "C" {
#include "psu_app.h"
}
void ScreenServiceView::allOff() { psu_app_shutdown(); }
