#ifndef SCREENUSBPDVIEW_HPP
#define SCREENUSBPDVIEW_HPP

#include <gui_generated/screenusbpd_screen/ScreenUsbPdViewBase.hpp>
#include <gui/screenusbpd_screen/ScreenUsbPdPresenter.hpp>
#include <stdint.h>

class ScreenUsbPdView : public ScreenUsbPdViewBase
{
public:
    ScreenUsbPdView();
    virtual ~ScreenUsbPdView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void usbAuto();
    virtual void usbSink();
    virtual void usbSource();
protected:
    uint8_t divider;
    void refresh();
};

#endif
