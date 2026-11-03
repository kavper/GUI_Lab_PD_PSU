#ifndef SCREENPROTECTIONVIEW_HPP
#define SCREENPROTECTIONVIEW_HPP
#include <gui_generated/screenprotection_screen/ScreenProtectionViewBase.hpp>
#include <gui/screenprotection_screen/ScreenProtectionPresenter.hpp>
#include <stdint.h>
#include <gui/common/Kit.hpp>
// Protection: status banner, four supervisor cards, active faults, link health.
class ScreenProtectionView : public ScreenProtectionViewBase {
public:
    ScreenProtectionView();
    virtual ~ScreenProtectionView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void clearFault();
    virtual void refreshTelemetry();
protected:
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
    void layout();
    void onClick(const kit::Btn& b) { if (b.tag == 1) clearFault(); else refreshTelemetry(); }
    kit::Header header;
    kit::Panel banner;
    kit::Btn bannerIcon, clearBtn, refreshBtn;
    kit::Label bannerTitle, bannerSub;
    kit::Metric status[4];
    kit::Card faultsCard, linkCard;
    kit::Rows faultRows, linkRows;
    touchgfx::Callback<ScreenProtectionView, const kit::Btn&> clicked;
};
#endif
