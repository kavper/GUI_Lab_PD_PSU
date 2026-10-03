#ifndef SCREENBMSVIEW_HPP
#define SCREENBMSVIEW_HPP
#include <gui_generated/screenbms_screen/ScreenBmsViewBase.hpp>
#include <gui/screenbms_screen/ScreenBmsPresenter.hpp>
#include <stdint.h>
class ScreenBmsView : public ScreenBmsViewBase {
public:
    void setupTheme();
    ScreenBmsView();
    virtual ~ScreenBmsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void initializeBms();
    virtual void refreshTelemetry();
protected:
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
};
#endif
