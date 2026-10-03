#include <texts/TextKeysAndLanguages.hpp>
#include <gui/screennetwork_screen/ScreenNetworkView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenNetworkView::ScreenNetworkView()
    : divider(0)
{
}

void ScreenNetworkView::setupScreen()
{
    ScreenNetworkViewBase::setupScreen();
    refresh();
}

void ScreenNetworkView::tearDownScreen()
{
    ScreenNetworkViewBase::tearDownScreen();
}

void ScreenNetworkView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenNetworkView::refresh()
{
    PsuSnapshot snap;
    char buf[24];
    const char* link;
    touchgfx::colortype link_color;
    psu_app_ensure();
    psu_snapshot(&snap);
    RemoteButton.setLabelText(touchgfx::TypedText(psu_remote_enabled()?T_TXT_REMOTE_DISABLE:T_TXT_REMOTE_ENABLE));RemoteButton.invalidate();
    lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,psu_remote_enabled()?"Remote access enabled / Ethernet and future Wi-Fi":"Remote access disabled / local HMI only",lab_cyan());
    lab_ip(buf, sizeof(buf), snap.ip);
    lab_show(NetIp, NetIpBuffer, NETIP_SIZE, buf, lab_cyan());
    lab_ip(buf, sizeof(buf), snap.mask);
    lab_show(NetMask, NetMaskBuffer, NETMASK_SIZE, buf, lab_text());
    lab_ip(buf, sizeof(buf), snap.gateway);
    lab_show(NetGw, NetGwBuffer, NETGW_SIZE, buf, lab_text());
    if (!snap.ethernet_configured)
    {
        link = "NO ETHERNET";
        link_color = lab_muted();
    }
    else if (snap.net_state == PSU_NET_BOUND)
    {
        link = "BOUND";
        link_color = lab_green();
    }
    else if (snap.net_state == PSU_NET_DHCP)
    {
        link = "DHCP";
        link_color = lab_amber();
    }
    else if (snap.net_state == PSU_NET_TIMEOUT)
    {
        link = "TIMEOUT";
        link_color = lab_amber();
    }
    else
    {
        link = "DOWN";
        link_color = lab_muted();
    }
    lab_show(NetLink, NetLinkBuffer, NETLINK_SIZE, link, link_color);
    lab_mac(buf, sizeof(buf), snap.mac);
    lab_show(NetMac, NetMacBuffer, NETMAC_SIZE, buf, lab_text());
}

extern "C" {
#include "psu_app.h"
}
void ScreenNetworkView::allOff() { psu_app_shutdown(); }

void ScreenNetworkView::toggleRemote(){psu_remote_enable(!psu_remote_enabled());refresh();}
