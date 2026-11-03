#include <gui/screenbms_screen/ScreenBmsView.hpp>
#include <gui/common/ErrorCatalog.hpp>
#include <gui/common/KitPage.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/CellBalance.hpp>
#include <stdlib.h>

ScreenBmsView::ScreenBmsView() : divider(0), noticeTicks(0), clicked(this, &ScreenBmsView::onClick) { notice[0] = 0; }

void ScreenBmsView::setupScreen()
{
    ScreenBmsViewBase::setupScreen();
    layout();
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this); theme.box(LabBackground, ui::BACKGROUND); theme.apply();
    refresh();
}
void ScreenBmsView::tearDownScreen() { ScreenBmsViewBase::tearDownScreen(); }
void ScreenBmsView::navigateBack() { kit::app()->navigateBack(); }

void ScreenBmsView::layout()
{
    KIT_PAGE_BEGIN(FrontendApplication::BMS);
    cellsCard.setup(12, 58, 428, 410, "CELLS");
    regCard.setup(450, 58, 338, 186, "SAFETY REGISTERS");
    deviceCard.setup(450, 254, 338, 214, "DEVICE");
    add(cellsCard); add(regCard); add(deviceCard);

    static const char* const heads[] = { "CELL", "DEVIATION FROM AVG", "VOLTAGE", "\xCE\x94 mV", "STATE" };
    static const int hx[] = { 25, 70, 196, 282, 340 }, hw[] = { 40, 120, 80, 50, 90 };
    static const kit::Align ha[] = { kit::LEFT, kit::LEFT, kit::RIGHT, kit::RIGHT, kit::LEFT };
    for (int i = 0; i < 5; ++i) {
        head[i].setup(kit::CAP, ha[i], hx[i], 88, hw[i], 14, ui::MUTED);
        head[i].set(heads[i]);
        add(head[i]);
    }
    for (int i = 0; i < CELLS; ++i) {
        const int y = 112 + i * 37;
        rule[i].setup(24, y - 4, 404, 1, ui::BORDER);
        cellName[i].setup(kit::M14, kit::LEFT, 25, y, 40, 30, ui::TEXT);
        char n[4] = { 'C', char('1' + i), 0, 0 };
        cellName[i].set(n);
        deviation[i].setup(70, y + 12, 116, 8, ui::ACCENT, ui::SURFACE);
        deviation[i].setCentered(true);
        cellValue[i].setup(kit::M16, kit::RIGHT, 196, y, 80, 30, ui::TEXT);
        cellDelta[i].setup(kit::M14, kit::RIGHT, 282, y, 50, 30, ui::MUTED);
        cellState[i].setup(kit::SUB, kit::LEFT, 344, y, 84, 30, ui::MUTED);
        add(rule[i]); add(cellName[i]); add(deviation[i]); add(cellValue[i]); add(cellDelta[i]); add(cellState[i]);
    }
    static const char* const foots[] = { "STACK", "SPREAD", "PACK I" };
    for (int i = 0; i < 3; ++i) {
        footCaption[i].setup(kit::CAP, kit::LEFT, 25 + i * 136, 314, 120, 14, ui::MUTED);
        footCaption[i].set(foots[i]);
        foot[i].setup(kit::M20, kit::M12, kit::LEFT, 24 + i * 136, 330, 130, 28, i == 0 ? ui::VOLT : i == 2 ? ui::AMP : ui::TEXT);
        add(footCaption[i]); add(foot[i]);
    }
    initBtn.setup(24, 412, 199, 44, "INITIALIZE BMS", kit::Btn::NORMAL, kit::BODYM);
    refreshBtn.setup(229, 412, 199, 44, "REFRESH", kit::Btn::NORMAL, kit::BODYM);
    initBtn.setBase(ui::SURFACE); refreshBtn.setBase(ui::SURFACE);
    initBtn.tag = 1; refreshBtn.tag = 2;
    initBtn.setAction(clicked); refreshBtn.setAction(clicked);
    add(initBtn); add(refreshBtn);

    static const char* const regs[] = { "Safety A", "Safety B", "Safety C", "Alarm", "Fault", "FET" };
    for (int i = 0; i < REGS; ++i) {
        reg[i].setup(462 + (i % 3) * 106, 86 + (i / 3) * 76, 100, 68, regs[i], kit::Btn::FIELD, kit::SUB);
        reg[i].setBase(ui::SURFACE); reg[i].setTouchable(false);
        add(reg[i]);
    }
    add(regChip);
    deviceRows.setup(463, 280, 312, 180, 22);
    add(deviceRows);
    KIT_PAGE_HEADER(FrontendApplication::BMS);
}

void ScreenBmsView::handleTickEvent()
{
    if (kit::app()->isScreenTransitionActive()) return;
    if (noticeTicks) --noticeTicks;
    if (++divider >= 4) { divider = 0; refresh(); }
    bars.step();
    for (int i = 0; i < CELLS; ++i) if (bars.usable(i)) deviation[i].set(bars.permille(i));
}
void ScreenBmsView::notify(const char* text) { snprintf(notice, sizeof(notice), "%s", text); noticeTicks = 180; refresh(); }
void ScreenBmsView::allOff() { psu_app_power_shutdown(); notify("POWER OFF requested - BMS shutdown / wake with TS2"); }
void ScreenBmsView::initializeBms() { notify(psu_app_bms_cmd("BMS") ? "BMS initialization requested; waiting for reported status" : "BMS request blocked"); }
void ScreenBmsView::refreshTelemetry() { notify(g4_simple(psu_g4(), "STATUS", 0) ? "PING sent - telemetry streams automatically" : "Command queue full"); }

void ScreenBmsView::refresh()
{
    TelemetryRecord pack(G4_RECORD_TB);
    const bool fresh = pack.fresh;
    const bool sample = pack.is("bms", 1) && pack.is("sample", 1);
    char b[96], rows[400];
    int64_t cells[CELLS] = { 0 }, balance = 0, series = 0, lo = 100000, hi = 0, sum = 0;
    bool usable[CELLS] = { false }, unused[CELLS] = { false };
    unsigned active = 0;
    pack.get("balance_mask", balance);
    for (int i = 0; i < CELLS; ++i) {
        char key[12]; snprintf(key, sizeof(key), "c%u_mv", i + 1);
        const bool have = pack.get(key, cells[i]);
        unused[i] = have && cells[i] == -1;
        usable[i] = sample && have && cells[i] > 0 && cells[i] <= 5000;
        if (usable[i]) { if (cells[i] < lo) lo = cells[i]; if (cells[i] > hi) hi = cells[i]; sum += cells[i]; ++active; }
    }
    const int64_t avg = active ? sum / active : 0;
    int64_t dev[CELLS];
    for (int i = 0; i < CELLS; ++i) dev[i] = cells[i] - avg;
    bars.target(usable, dev);
    const long scale = (long)bars.scaleMv();
    for (int i = 0; i < CELLS; ++i) {
        if (usable[i]) {
            kit::milli(b, sizeof(b), cells[i], "", 3);
            cellValue[i].set(b, ui::TEXT);
            const int64_t d = cells[i] - avg;
            snprintf(b, sizeof(b), "%s%ld", d > 0 ? "+" : "", (long)d);
            cellDelta[i].set(b, ui::MUTED);
            deviation[i].set(bars.permille(i),cells[i] == lo && lo != hi ? ui::CAUTION : ui::ACCENT);
            const bool bal = balance & (1U << i);
            const char* st = bal ? "balancing" : (lo != hi && cells[i] == lo) ? "min" : (lo != hi && cells[i] == hi) ? "max" : "";
            cellState[i].set(st, bal ? ui::POSITIVE : ui::MUTED);
            cellName[i].setRole(ui::TEXT);
        } else {
            cellValue[i].set(unused[i] ? "unused" : "--", ui::MUTED);
            cellDelta[i].set("", ui::MUTED);
            deviation[i].set(0, ui::MUTED);
            cellState[i].set(unused[i] ? "inactive" : "no data", ui::MUTED);
            cellName[i].setRole(ui::MUTED);
        }
    }
    if (pack.get("series", series) && series > 0) snprintf(b, sizeof(b), "%ldS \xC2\xB7 avg %ld.%03ld V \xC2\xB7 bars +/-%ld mV", (long)series, (long)(avg / 1000), (long)(avg % 1000), scale);
    else snprintf(b, sizeof(b), active ? "avg %ld.%03ld V \xC2\xB7 bars +/-%ld mV" : "waiting for cell samples", (long)(avg / 1000), (long)(avg % 1000), scale);
    cellsCard.setNote(b);
    int64_t v = 0;
    if (sample && pack.get("stack_mv", v)) { kit::milli(b, sizeof(b), v, "V"); foot[0].set(b, ui::VOLT); } else foot[0].set("-- V", ui::MUTED);
    if (active) { snprintf(b, sizeof(b), "%ld mV", (long)(hi - lo)); foot[1].set(b, hi - lo > 50 ? ui::CAUTION : ui::TEXT); } else foot[1].set("-- mV", ui::MUTED);
    if (sample && pack.get("i_pack_ma", v)) { kit::milli(b, sizeof(b), v, "A"); if (v > 0) { memmove(b + 1, b, strlen(b) + 1); b[0] = '+'; } foot[2].set(b, ui::AMP); } else foot[2].set("-- A", ui::MUTED);

    static const char* const keys[] = { "sa", "sb", "sc", "alarm", "fault", "fet" };
    static const err::GroupId groups[] = { err::E_BMS_SA, err::E_BMS_SB, err::E_BMS_SC, err::E_BMS_ALARM, err::E_BMS_FAULT };
    char issue[64];
    const int severity = err::bmsIssue(pack, issue, sizeof(issue));
    bool anyBad = severity >= err::ES_WARNING;
    for (int i = 0; i < REGS; ++i) {
        int64_t x;
        if (pack.get(keys[i], x)) {
            char set[40] = "";
            // Event/info bits (scan done, wake-up...) are not problems.
            if (i < 5) err::symbols(groups[i], (uint32_t)x, set, sizeof(set));
            const bool bad = set[0] != 0;
            if (i == 5) snprintf(b, sizeof(b), "0x%02lX %s%s", (unsigned long)x, (x & 1) ? "CHG" : "", (x & 2) ? "+DSG" : "");
            else if (bad) snprintf(b, sizeof(b), "%s", set);
            else snprintf(b, sizeof(b), "0x%02lX ok", (unsigned long)x);
            reg[i].setDetail(b, kit::M14);
            reg[i].setInk(bad ? ui::NEGATIVE : i == 5 ? ui::TEXT : ui::POSITIVE);
        } else { reg[i].setDetail("--", kit::M14); reg[i].setInk(ui::MUTED); }
    }
    regChip.set(!fresh ? "OFFLINE" : anyBad ? "ATTENTION" : "ALL CLEAR", -1, !fresh ? ui::MUTED : anyBad ? ui::NEGATIVE : ui::POSITIVE);
    regChip.setXY(776 - regChip.getWidth(), 64);

    rows[0] = 0;
    pack.row(rows, sizeof(rows), "Init step", "init_step", "", 1, 0);
    pack.hex(rows, sizeof(rows), "VCell mode", "vcell_rb");
    pack.hex(rows, sizeof(rows), "Battery status", "batt");
    pack.row(rows, sizeof(rows), "Config failures", "cfg_fail", "", 1, 0);
    pack.row(rows, sizeof(rows), "I2C errors", "i2c_err", "", 1, 0);
    pack.row(rows, sizeof(rows), "Alert count", "alerts", "", 1, 0);
    pack.flag(rows, sizeof(rows), "Configuration", "cfg", "READY", "NOT READY");
    deviceRows.set(rows, fresh);
    initBtn.setEnabled(fresh);

    if (noticeTicks) header.setSub(notice, ui::TEXT);
    else if (!pack.record.valid) header.setSub("Waiting for BMS telemetry", ui::CAUTION);
    else if (fresh && severity >= err::ES_WARNING) header.setSub(issue, severity >= err::ES_FAULT ? ui::NEGATIVE : ui::CAUTION);
    else { snprintf(b, sizeof(b), "%s %lu ms ago", fresh ? "updated" : "STALE", (unsigned long)(psu_app_now() - pack.record.ms)); header.setSub(b, fresh ? ui::MUTED : ui::CAUTION); }
}
