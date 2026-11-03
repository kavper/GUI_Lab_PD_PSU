#ifndef SCREENBMSVIEW_HPP
#define SCREENBMSVIEW_HPP
#include <gui_generated/screenbms_screen/ScreenBmsViewBase.hpp>
#include <gui/screenbms_screen/ScreenBmsPresenter.hpp>
#include <stdint.h>
#include <gui/common/Kit.hpp>
// Cells & BMS: cell table with deviation bars, safety registers, device info.
class ScreenBmsView : public ScreenBmsViewBase {
public:
    ScreenBmsView();
    virtual ~ScreenBmsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void allOff();
    virtual void initializeBms();
    virtual void refreshTelemetry();
    virtual void navigateBack();
protected:
    enum { CELLS = 5, REGS = 6 };
    uint8_t divider;
    uint16_t noticeTicks;
    char notice[100];
    void refresh();
    void notify(const char* text);
    void layout();
    void onClick(const kit::Btn& b) { if (b.tag == 1) initializeBms(); else refreshTelemetry(); }
    kit::Header header;
    kit::Card cellsCard, regCard, deviceCard;
    kit::Label head[5], cellName[CELLS], cellValue[CELLS], cellDelta[CELLS], cellState[CELLS], footCaption[3];
    kit::Fill rule[CELLS];
    kit::Bar deviation[CELLS];
    kit::Value foot[3];
    kit::Btn reg[REGS], initBtn, refreshBtn;
    kit::Chip regChip;
    kit::Rows deviceRows;
    touchgfx::Callback<ScreenBmsView, const kit::Btn&> clicked;
};
#endif
