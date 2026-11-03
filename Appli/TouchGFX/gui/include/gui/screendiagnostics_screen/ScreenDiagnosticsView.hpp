#pragma once
#include <gui_generated/screendiagnostics_screen/ScreenDiagnosticsViewBase.hpp>
#include <gui/screendiagnostics_screen/ScreenDiagnosticsPresenter.hpp>
#include <gui/common/Kit.hpp>
// Diagnostics: live telemetry fields per G4 record (parsed or raw UART), link
// counters and the last event.
class ScreenDiagnosticsView : public ScreenDiagnosticsViewBase {
public:
    ScreenDiagnosticsView();
    virtual void setupScreen();
    virtual void tearDownScreen() { ScreenDiagnosticsViewBase::tearDownScreen(); }
    virtual void handleTickEvent();
    virtual void handleDragEvent(const touchgfx::DragEvent& e);
    virtual void allOff();
    virtual void clearFault();
private:
    unsigned kind, offset;
    bool raw;
    unsigned divider;
    int drag;
    kit::Header header;
    kit::Panel tabs;
    kit::Btn tab[6], counter[4], clearBtn;
    kit::Card fieldsLeft, fieldsRight, linkCard, eventCard;
    kit::Rows left, right, event;
    touchgfx::Callback<ScreenDiagnosticsView, const kit::Btn&> clicked;
    void layout();
    void refresh();
    void onClick(const kit::Btn& b);
};
