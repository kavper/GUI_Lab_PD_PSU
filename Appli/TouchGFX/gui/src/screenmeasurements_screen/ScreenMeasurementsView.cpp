#include <gui/screenmeasurements_screen/ScreenMeasurementsView.hpp>
#include <gui/common/KitPage.hpp>
#include <gui/common/UiTheme.hpp>

ScreenMeasurementsView::ScreenMeasurementsView() : divider(0), noticeTicks(0), clicked(this, &ScreenMeasurementsView::onClick) { notice[0] = 0; }

void ScreenMeasurementsView::setupScreen()
{
    ScreenMeasurementsViewBase::setupScreen();
    layout();
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this); theme.box(LabBackground, ui::BACKGROUND); theme.apply();
    refresh();
}
void ScreenMeasurementsView::tearDownScreen() { ScreenMeasurementsViewBase::tearDownScreen(); }

void ScreenMeasurementsView::layout()
{
    KIT_PAGE_BEGIN(FrontendApplication::OUTPUT);
    static const char* const captions[] = { "INPUT", "BUCK-BOOST", "DCDC RAIL", "LDO", "OUTPUT" };
    for (int i = 0; i < 5; ++i) {
        const int x = 12 + i * 164;
        stage[i].setup(x, 58, 122, 118, captions[i]);
        add(stage[i]);
        stageValue[i].setup(kit::M20, kit::M12, kit::LEFT, x + 12, 78, 98, 28, ui::TEXT);
        stageSub[i][0].setup(kit::SUB, kit::LEFT, x + 12, i == 1 ? 124 : 110, 106, 16, ui::MUTED);
        stageSub[i][1].setup(kit::SUB, kit::LEFT, x + 12, i == 1 ? 140 : i == 4 ? 134 : 126, 106, 16, ui::MUTED);
        add(stageValue[i]); add(stageSub[i][0]); add(stageSub[i][1]);
        if (i < 4) {
            arrow[i].setup(kit::HEAD, kit::CENTER, x + 122, 104, 42, 28, ui::MUTED);
            arrow[i].set("\xE2\x86\x92");
            add(arrow[i]);
        }
    }
    stageValue[1].setup(kit::LABEL, kit::M12, kit::LEFT, 188, 78, 98, 28, ui::TEXT);
    stageValue2.setup(kit::M16, kit::M12, kit::LEFT, 678, 106, 98, 24, ui::AMP);
    add(stageValue2);
    dutyBar.setup(188, 112, 98, 5, ui::ACCENT);
    add(dutyBar);

    switching.setup(12, 186, 252, 282, "SWITCHING STAGE");
    regulation.setup(274, 186, 252, 282, "RAIL REGULATION");
    efficiency.setup(536, 186, 252, 282, "LDO EFFICIENCY");
    add(switching); add(regulation); add(efficiency);
    switchRows.setup(25, 212, 226, 200);
    regRows.setup(287, 212, 226, 200);
    effValue.setup(kit::M34, kit::M16, kit::LEFT, 548, 208, 220, 46, ui::ACCENT);
    effRows.setup(549, 262, 226, 80);
    add(switchRows); add(regRows); add(effValue); add(effRows);
    refreshBtn.setup(548, 412, 228, 44, "REFRESH", kit::Btn::NORMAL, kit::BODYM);
    refreshBtn.setBase(ui::SURFACE); refreshBtn.setAction(clicked);
    add(refreshBtn);
    KIT_PAGE_HEADER(FrontendApplication::OUTPUT);
}

void ScreenMeasurementsView::handleTickEvent()
{
    if (kit::app()->isScreenTransitionActive()) return;
    if (noticeTicks) --noticeTicks;
    if (++divider >= 4) { divider = 0; refresh(); }
}
void ScreenMeasurementsView::notify(const char* text) { snprintf(notice, sizeof(notice), "%s", text); noticeTicks = 180; refresh(); }
void ScreenMeasurementsView::allOff() { psu_app_power_shutdown(); notify("POWER OFF requested - BMS shutdown / wake with TS2"); }
void ScreenMeasurementsView::refreshTelemetry() { notify(g4_simple(psu_g4(), "STATUS", 0) ? "PING sent - telemetry streams automatically" : "Command queue full"); }

void ScreenMeasurementsView::refresh()
{
    TelemetryRecord r(G4_RECORD_T), usb(G4_RECORD_TC), pack(G4_RECORD_TB);
    PsuSnapshot live; psu_snapshot(&live);
    char b[64], c[64];
    int64_t vin = 0, rail = 0, dutyA = 0, dutyC = 0, req = 0, iout = 0, v = 0;

    // Input.
    if (r.get("vin_mv", vin)) { kit::milli(b, sizeof(b), vin, "V", 2); stageValue[0].set(b, ui::VOLT); } else stageValue[0].set("-- V", ui::MUTED);
    if (usb.get("pd_mv", v)) { kit::milli(c, sizeof(c), v, "V", 2); snprintf(b, sizeof(b), "USB-C %s", c); } else snprintf(b, sizeof(b), "USB-C --");
    stageSub[0][0].set(b);
    if (pack.get("pack_mv", v)) { kit::milli(c, sizeof(c), v, "V", 2); snprintf(b, sizeof(b), "bat %s", c); } else snprintf(b, sizeof(b), "bat --");
    stageSub[0][1].set(b);

    // Buck-boost.
    const bool haveDuty = r.get("duty_a_x10", dutyA) && r.get("duty_c_x10", dutyC);
    const bool stageOn = r.is("stage_en", 1);
    const char* mode = !haveDuty ? "--" : !stageOn ? "OFF" : dutyC > 0 ? "BOOST" : dutyA >= 995 ? "PASS" : "BUCK";
    stageValue[1].set(mode, !haveDuty ? ui::MUTED : stageOn ? ui::TEXT : ui::MUTED);
    dutyBar.set(haveDuty ? (int)(dutyC > 0 ? dutyC : dutyA) : 0, ui::ACCENT);
    kit::deci(c, sizeof(c), dutyA, "%"); snprintf(b, sizeof(b), "duty A %s", haveDuty ? c : "--"); stageSub[1][0].set(b);
    kit::deci(c, sizeof(c), dutyC, "%"); snprintf(b, sizeof(b), "duty C %s", haveDuty ? c : "--"); stageSub[1][1].set(b);
    stage[1].setBorder(stageOn ? ui::ACCENT : ui::BORDER);

    // DCDC rail.
    const bool haveRail = r.get("vout_mv", rail);
    if (haveRail) { kit::milli(b, sizeof(b), rail, "V", 2); stageValue[2].set(b, ui::VOLT); } else stageValue[2].set("-- V", ui::MUTED);
    if (r.get("vpre_req_mv", req)) { kit::milli(c, sizeof(c), req, "V", 2); snprintf(b, sizeof(b), "target %s", c); } else snprintf(b, sizeof(b), "target --");
    stageSub[2][0].set(b);
    stageSub[2][1].set(!r.fresh ? "--" : r.is("reg_ok", 1) ? "regulating" : "settling", r.is("reg_ok", 1) ? ui::POSITIVE : ui::CAUTION);

    // LDO and output (G0 readback).
    const bool outValid = live.g0_connected && !live.g0_stale;
    const int64_t ua = live.current_valid && live.display_current_ua > 0 ? live.display_current_ua : 0;
    if (haveRail && outValid) {
        const int64_t head = rail - (int64_t)live.vout_mv;
        kit::milli(b, sizeof(b), head > 0 ? head : 0, "V", 2);
        stageValue[3].set(b, ui::TEXT);
        const int64_t lossMw = head > 0 ? head * ua / 1000000 : 0;
        snprintf(b, sizeof(b), "%s \xC2\xB7 %ld.%02ld W", r.is("permit", 1) ? "permit" : "blocked", (long)(lossMw / 1000), (long)(lossMw % 1000 / 10));
        stageSub[3][1].set(b, r.is("permit", 1) ? ui::POSITIVE : ui::CAUTION);
    } else { stageValue[3].set("-- V", ui::MUTED); stageSub[3][1].set("--", ui::MUTED); }
    stageSub[3][0].set("headroom");
    if (outValid) { kit::milli(b, sizeof(b), live.vout_mv, "V", 3); stageValue[4].set(b, ui::VOLT); } else stageValue[4].set("-- V", ui::MUTED);
    if (outValid && live.current_valid) { kit::milli(b, sizeof(b), ua / 1000, "A", 3); stageValue2.set(b, ui::AMP); } else stageValue2.set("-- A", ui::MUTED);
    stageSub[4][1].set(live.output_confirmed ? (live.mode_cc ? "ON \xC2\xB7 CC" : "ON \xC2\xB7 CV") : "OFF", live.output_confirmed ? ui::POSITIVE : ui::MUTED);
    stage[4].setBorder(live.output_confirmed ? ui::POSITIVE : ui::BORDER);

    // Cards.
    char rows[400];
    rows[0] = 0;
    r.row(rows, sizeof(rows), "Buck PWM (TA1)", "duty_a_x10", " %", 10, 1);
    r.row(rows, sizeof(rows), "Boost PWM (TC2)", "duty_c_x10", " %", 10, 1);
    r.flag(rows, sizeof(rows), "Buck aux", "ucc_a");
    r.flag(rows, sizeof(rows), "Boost aux", "ucc_c");
    r.row(rows, sizeof(rows), "Buck current", "i_buck_ma", " A", 1000, 3);
    r.row(rows, sizeof(rows), "Boost current", "i_boost_ma", " A", 1000, 3);
    r.row(rows, sizeof(rows), "DCDC current", "iout_ma", " A", 1000, 3);
    switchRows.set(rows, r.fresh);
    rows[0] = 0;
    r.row(rows, sizeof(rows), "Rail target", "vpre_req_mv", " V", 1000, 3);
    r.row(rows, sizeof(rows), "Slew command", "vpre_cmd_mv", " V", 1000, 3);
    r.flag(rows, sizeof(rows), "Regulation", "reg_ok", "LOCKED", "SETTLING");
    r.flag(rows, sizeof(rows), "Power stage", "stage_en");
    r.flag(rows, sizeof(rows), "Driver fault", "flt", "ACTIVE", "CLEAR");
    r.row(rows, sizeof(rows), "Start hold", "hold_ms", " ms", 1, 0);
    regRows.set(rows, r.fresh);

    // LDO stage efficiency: DCDC rail power in, output power out.
    rows[0] = 0;
    if (haveRail && r.get("iout_ma", iout) && outValid && live.current_valid && iout > 0) {
        const int64_t inMw = rail * iout / 1000, outMw = (int64_t)live.vout_mv * ua / 1000000;
        const int64_t pct = inMw > 0 ? outMw * 100 / inMw : 0;
        snprintf(b, sizeof(b), "%ld %%", (long)(pct > 100 ? 100 : pct));
        effValue.set(b, ui::ACCENT);
        kit::milli(c, sizeof(c), inMw, "W", 2); TelemetryRecord::append(rows, sizeof(rows), "In (rail)", c);
        kit::milli(c, sizeof(c), outMw, "W", 2); TelemetryRecord::append(rows, sizeof(rows), "Out", c);
        kit::milli(c, sizeof(c), inMw - outMw, "W", 2); TelemetryRecord::append(rows, sizeof(rows), "Loss", c);
    } else {
        effValue.set("-- %", ui::MUTED);
        TelemetryRecord::append(rows, sizeof(rows), "In (rail)", "--");
        TelemetryRecord::append(rows, sizeof(rows), "Out", "--");
        TelemetryRecord::append(rows, sizeof(rows), "Loss", "--");
    }
    effRows.set(rows, true);

    if (noticeTicks) header.setSub(notice, ui::TEXT);
    else if (!r.record.valid) header.setSub("Waiting for METER telemetry", ui::CAUTION);
    else { snprintf(b, sizeof(b), "DCDC + LDO \xC2\xB7 %s %lu ms ago", r.fresh ? "updated" : "STALE", (unsigned long)(psu_app_now() - r.record.ms)); header.setSub(b, r.fresh ? ui::MUTED : ui::CAUTION); }
}
