#include <gui/screennetwork_screen/ScreenNetworkView.hpp>
#include <gui/common/KitPage.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenNetworkView::ScreenNetworkView() : divider(0), clicked(this, &ScreenNetworkView::onClick) {}

void ScreenNetworkView::setupScreen()
{
    ScreenNetworkViewBase::setupScreen();
    layout();
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this); theme.box(LabBackground, ui::BACKGROUND); theme.apply();
    refresh();
}

void ScreenNetworkView::layout()
{
    KIT_PAGE_BEGIN(FrontendApplication::NETWORK);
    consoleCard.setup(12, 58, 776, 112, "");
    addressCard.setup(12, 180, 384, 288, "ADDRESS");
    remoteCard.setup(406, 180, 382, 288, "");
    add(consoleCard); add(addressCard); add(remoteCard);
    consoleIcon.setup(24, 76, 52, 76, "", kit::Btn::GHOST);
    consoleIcon.setBase(ui::SURFACE); consoleIcon.setIcon(kit::ICON_MONITOR); consoleIcon.setIconSize(24);
    consoleIcon.setInk(ui::ACCENT); consoleIcon.setTouchable(false);
    add(consoleIcon);
    consoleCaption.setup(kit::CAP, kit::LEFT, 86, 76, 300, 14, ui::MUTED);
    consoleCaption.set("WEB CONSOLE");
    url.setup(kit::M26, kit::LEFT, 84, 94, 560, 36, ui::ACCENT);
    consoleSub.setup(kit::SUB, kit::LEFT, 86, 134, 560, 16, ui::MUTED);
    add(consoleCaption); add(url); add(consoleSub); add(linkChip);
    addressRows.setup(25, 206, 358, 140, 26);
    addressNote.setup(kit::SUB, kit::LEFT, 25, 444, 358, 16, ui::MUTED);
    addressNote.set("Wi-Fi module not fitted");
    add(addressRows); add(addressNote);
    remoteTitle.setup(kit::LABEL, kit::LEFT, 420, 194, 260, 22, ui::TEXT);
    remoteTitle.set("Remote control");
    remoteText.setup(kit::SUB, kit::LEFT, 420, 222, 356, 64, ui::MUTED);
    add(remoteTitle); add(remoteText);
    remoteSwitch.setup(692, 190, 84, 36, "OFF", kit::Btn::NORMAL, kit::BODYM);
    remoteSwitch.setBase(ui::SURFACE); remoteSwitch.setAction(clicked);
    add(remoteSwitch);
    KIT_PAGE_HEADER(FrontendApplication::NETWORK);
}

void ScreenNetworkView::handleTickEvent()
{
    if (kit::app()->isScreenTransitionActive()) return;
    if (++divider < 8) return;
    divider = 0;
    refresh();
}

void ScreenNetworkView::refresh()
{
    PsuSnapshot snap;
    psu_app_ensure();
    psu_snapshot(&snap);
    char ip[24], b[80], rows[400];
    lab_ip(ip, sizeof(ip), snap.ip);
    const bool bound = snap.ethernet_configured && snap.net_state == PSU_NET_BOUND;
    if (bound) { snprintf(b, sizeof(b), "http://%s", ip); url.set(b, ui::ACCENT); }
    else url.set(!snap.ethernet_configured ? "Ethernet not configured" : snap.net_state == PSU_NET_DHCP ? "Waiting for DHCP..." : "No network link", ui::MUTED);
    consoleSub.set(bound ? "Open in any browser on the same network \xC2\xB7 DHCP" : "Local controls stay available without a network");
    const char* link = !snap.ethernet_configured ? "NOT READY" : bound ? "LINK UP" : snap.net_state == PSU_NET_DHCP ? "DHCP" : snap.net_state == PSU_NET_TIMEOUT ? "TIMEOUT" : "DOWN";
    linkChip.set(link, bound ? ui::POSITIVE : ui::MUTED, bound ? ui::POSITIVE : ui::MUTED);
    linkChip.setXY(776 - linkChip.getWidth(), 76);

    rows[0] = 0;
    TelemetryRecord::append(rows, sizeof(rows), "IP", bound ? ip : "--");
    lab_ip(b, sizeof(b), snap.mask); TelemetryRecord::append(rows, sizeof(rows), "Mask", bound ? b : "--");
    lab_ip(b, sizeof(b), snap.gateway); TelemetryRecord::append(rows, sizeof(rows), "Gateway", bound ? b : "--");
    lab_mac(b, sizeof(b), snap.mac); TelemetryRecord::append(rows, sizeof(rows), "MAC", b);
    TelemetryRecord::append(rows, sizeof(rows), "Link", link);
    addressRows.set(rows, true);

    const bool remote = psu_remote_enabled();
    remoteSwitch.setLabel(remote ? "ON" : "OFF");
    remoteSwitch.setStyle(remote ? kit::Btn::POSITIVE_FILL : kit::Btn::NORMAL);
    remoteText.set("Browsers can change setpoints and the\noutput; write access ends 10 s after\nthe last command.");
    header.setSub(!snap.ethernet_configured ? "Ethernet bring-up pending \xC2\xB7 local controls available" : remote ? "LAB-PD-PSU \xC2\xB7 remote control enabled" : "LAB-PD-PSU \xC2\xB7 remote control disabled",
                  ui::MUTED);
}

void ScreenNetworkView::allOff() { psu_app_power_shutdown(); }
void ScreenNetworkView::toggleRemote() { psu_remote_enable(!psu_remote_enabled()); refresh(); }
