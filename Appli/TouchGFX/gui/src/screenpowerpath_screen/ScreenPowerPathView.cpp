#include <gui/screenpowerpath_screen/ScreenPowerPathView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenPowerPathView::ScreenPowerPathView()
    : divider(0)
{
}

void ScreenPowerPathView::setupScreen()
{
    ScreenPowerPathViewBase::setupScreen();
    refresh();
}

void ScreenPowerPathView::tearDownScreen()
{
    ScreenPowerPathViewBase::tearDownScreen();
}

void ScreenPowerPathView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenPowerPathView::refresh()
{
    PsuSnapshot snap;
    psu_app_ensure();
    psu_snapshot(&snap);
    lab_show(PathLink, PathLinkBuffer, PATHLINK_SIZE, lab_g4_link(snap.g4_link), lab_link_color(snap.g4_link));
    lab_show(PathPps, PathPpsBuffer, PATHPPS_SIZE, snap.pps_allowed ? "OPEN" : "LOCKED",
             snap.pps_allowed ? lab_green() : lab_amber());
    lab_show(PathCmd, PathCmdBuffer, PATHCMD_SIZE, snap.g4_last_tx[0] ? snap.g4_last_tx : "--", lab_cyan());
}

void ScreenPowerPathView::pathPermit()
{
    g4_permit(psu_g4(), 1, 0);
    refresh();
}
void ScreenPowerPathView::pathRemote()
{
    g4_remote(psu_g4(), 1, 0);
    refresh();
}
