#include <gui/screenservice_screen/ScreenServiceView.hpp>
#include <gui/common/LabText.hpp>
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
    lab_show(SvcMode, SvcModeBuffer, SVCMODE_SIZE, snap.service_mode ? "OPEN" : "LOCKED",
             snap.service_mode ? lab_green() : lab_amber());
    lab_show(SvcTx, SvcTxBuffer, SVCTX_SIZE, snap.g4_last_tx[0] ? snap.g4_last_tx : "--", lab_cyan());
    lab_show(SvcRx, SvcRxBuffer, SVCRX_SIZE, snap.g4_last_rx[0] ? snap.g4_last_rx : "--", lab_text());
}

void ScreenServiceView::serviceToggle()
{
    psu_app_service(!psu_g4()->service_mode);
    refresh();
}
void ScreenServiceView::serviceHelp()
{
    psu_app_service_cmd("HELP");
    refresh();
}
void ScreenServiceView::serviceStatus()
{
    psu_app_service_cmd("STATUS");
    refresh();
}
