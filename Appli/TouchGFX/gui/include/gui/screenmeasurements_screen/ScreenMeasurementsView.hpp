#ifndef SCREENMEASUREMENTSVIEW_HPP
#define SCREENMEASUREMENTSVIEW_HPP
#include <gui_generated/screenmeasurements_screen/ScreenMeasurementsViewBase.hpp>
#include <gui/screenmeasurements_screen/ScreenMeasurementsPresenter.hpp>
#include <stdint.h>
class ScreenMeasurementsView : public ScreenMeasurementsViewBase {
public:
    void setupTheme();
    ScreenMeasurementsView();
    virtual ~ScreenMeasurementsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void refreshTelemetry();
protected:
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
};
#endif
