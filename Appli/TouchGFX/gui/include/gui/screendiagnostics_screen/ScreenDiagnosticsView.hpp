#pragma once
#include <gui_generated/screendiagnostics_screen/ScreenDiagnosticsViewBase.hpp>
#include <gui/screendiagnostics_screen/ScreenDiagnosticsPresenter.hpp>
class ScreenDiagnosticsView : public ScreenDiagnosticsViewBase {
public:
    void setupTheme();
 ScreenDiagnosticsView():kind(0),offset(0),raw(false),divider(0),drag(0){}
 virtual void setupScreen(){ScreenDiagnosticsViewBase::setupScreen();refresh();setupTheme();}
 virtual void tearDownScreen(){ScreenDiagnosticsViewBase::tearDownScreen();}
 virtual void handleTickEvent(){if(++divider>=12){divider=0;refresh();}}
 virtual void handleDragEvent(const touchgfx::DragEvent& e);
 virtual void allOff();
 virtual void showParsed();
 virtual void showRaw();
 virtual void frameT();
 virtual void frameTB();
 virtual void frameTC();
 virtual void scrollUp();
 virtual void scrollDown();
 virtual void clearFault();

private:
 unsigned kind,offset;bool raw;unsigned divider;int drag;
 void refresh();void frame(unsigned k){kind=k;offset=0;refresh();}
};
