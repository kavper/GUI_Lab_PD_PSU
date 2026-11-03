#ifndef SCREENNETWORKVIEW_HPP
#define SCREENNETWORKVIEW_HPP
#include <gui_generated/screennetwork_screen/ScreenNetworkViewBase.hpp>
#include <gui/screennetwork_screen/ScreenNetworkPresenter.hpp>
#include <gui/common/Kit.hpp>
#include <stdint.h>
// Network: web console address, interface details and remote control switch.
class ScreenNetworkView : public ScreenNetworkViewBase
{
public:
    ScreenNetworkView();
    virtual ~ScreenNetworkView() {}
    virtual void setupScreen();
    virtual void tearDownScreen() { ScreenNetworkViewBase::tearDownScreen(); }
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void toggleRemote();
protected:
    uint8_t divider;
    void refresh();
    void layout();
    void onClick(const kit::Btn&) { toggleRemote(); }
    kit::Header header;
    kit::Card consoleCard, addressCard, remoteCard;
    kit::Btn consoleIcon, remoteSwitch;
    kit::Label consoleCaption, url, consoleSub, remoteTitle, remoteText, addressNote;
    kit::Chip linkChip;
    kit::Rows addressRows;
    touchgfx::Callback<ScreenNetworkView, const kit::Btn&> clicked;
};
#endif
