#ifndef SCREENDIAGNOSTICSVIEW_HPP
#define SCREENDIAGNOSTICSVIEW_HPP

#include <gui_generated/screendiagnostics_screen/ScreenDiagnosticsViewBase.hpp>
#include <gui/screendiagnostics_screen/ScreenDiagnosticsPresenter.hpp>

class ScreenDiagnosticsView : public ScreenDiagnosticsViewBase
{
public:
    ScreenDiagnosticsView();
    virtual ~ScreenDiagnosticsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
protected:
    uint8_t refreshDivider;
};

#endif // SCREENDIAGNOSTICSVIEW_HPP
