#include <gui/screenmeasurements_screen/ScreenMeasurementsView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenMeasurementsView::ScreenMeasurementsView()
    : divider(0)
{
}

void ScreenMeasurementsView::setupScreen()
{
    ScreenMeasurementsViewBase::setupScreen();
    refresh();
}

void ScreenMeasurementsView::tearDownScreen()
{
    ScreenMeasurementsViewBase::tearDownScreen();
}

void ScreenMeasurementsView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenMeasurementsView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_measurements(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), MeasureBodyBuffer, MEASUREBODY_SIZE);
    MeasureBody.invalidate();
}


