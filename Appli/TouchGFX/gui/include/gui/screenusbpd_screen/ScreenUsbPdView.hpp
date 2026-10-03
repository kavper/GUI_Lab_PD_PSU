#ifndef SCREENUSBPDVIEW_HPP
#define SCREENUSBPDVIEW_HPP
#include <gui_generated/screenusbpd_screen/ScreenUsbPdViewBase.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdPresenter.hpp>
#include <stdint.h>
class ScreenUsbPdView : public ScreenUsbPdViewBase {
public:
    void setupTheme();
    ScreenUsbPdView();
    virtual ~ScreenUsbPdView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    void saveSwipeNotice(char* text,uint16_t& ticks) const;
    void restoreSwipeNotice(const char* text,uint16_t ticks);
    virtual void allOff();
    virtual void roleAuto();
    virtual void roleSink();
    virtual void roleSource();
    virtual void refreshTelemetry();
protected:
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
};
#endif
