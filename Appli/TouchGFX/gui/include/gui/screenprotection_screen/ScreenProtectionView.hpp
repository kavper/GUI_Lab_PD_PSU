#ifndef SCREENPROTECTIONVIEW_HPP
#define SCREENPROTECTIONVIEW_HPP
#include <gui_generated/screenprotection_screen/ScreenProtectionViewBase.hpp>
#include <gui/screenprotection_screen/ScreenProtectionPresenter.hpp>
#include <stdint.h>
class ScreenProtectionView : public ScreenProtectionViewBase {
public:
    void setupTheme();
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
};
#endif
