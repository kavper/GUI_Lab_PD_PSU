#include <gui/screenpowerpath_screen/ScreenPowerPathView.hpp>
#include <gui/common/ErrorCatalog.hpp>
#include <gui/common/KitPage.hpp>
#include <gui/common/UiTheme.hpp>

ScreenPowerPathView::ScreenPowerPathView() : divider(0), noticeTicks(0), remoteOn(false), clicked(this, &ScreenPowerPathView::onClick) { notice[0] = 0; }

void ScreenPowerPathView::setupScreen()
{
    ScreenPowerPathViewBase::setupScreen();
    layout();
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this); theme.box(LabBackground, ui::BACKGROUND); theme.apply();
    refresh();
}
void ScreenPowerPathView::tearDownScreen() { ScreenPowerPathViewBase::tearDownScreen(); }

void ScreenPowerPathView::layout()
{
    KIT_PAGE_BEGIN(FrontendApplication::SENSE);
    senseCard.setup(12, 58, 428, 410, "REMOTE SENSE");
    tempCard.setup(450, 58, 338, 300, "TEMPERATURES", "T1-T4");
    fanCard.setup(450, 368, 338, 100, "");
    add(senseCard); add(tempCard); add(fanCard);

    segment.setup(24, 84, 404, 46, ui::RAISED, ui::BORDER, ui::SURFACE, 8);
    add(segment);
    modeLocal.setup(28, 88, 197, 38, "LOCAL", kit::Btn::SEGMENT, kit::BODYM);
    modeRemote.setup(227, 88, 197, 38, "REMOTE 4-WIRE", kit::Btn::SEGMENT, kit::BODYM);
    modeLocal.setBase(ui::RAISED); modeRemote.setBase(ui::RAISED);
    modeLocal.tag = 1; modeRemote.tag = 2;
    modeLocal.setAction(clicked); modeRemote.setAction(clicked);
    add(modeLocal); add(modeRemote);
    localBox.setup(24, 140, 199, 56, "Local terminals", kit::Btn::FIELD, kit::SUB);
    loadBox.setup(229, 140, 199, 56, "At load (remote)", kit::Btn::FIELD, kit::SUB);
    localBox.setBase(ui::SURFACE); loadBox.setBase(ui::SURFACE);
    localBox.setTouchable(false); loadBox.setTouchable(false);
    localBox.setInk(ui::VOLT); loadBox.setInk(ui::VOLT);
    add(localBox); add(loadBox);
    senseRows.setup(25, 208, 402, 170, 22);
    senseRows.setPlainUnits(true);
    add(senseRows);
    permitBtn.setup(24, 372, 199, 44, "LDO PERMIT", kit::Btn::NORMAL, kit::BODYM);
    refreshBtn.setup(229, 372, 199, 44, "REFRESH", kit::Btn::NORMAL, kit::BODYM);
    permitBtn.setBase(ui::SURFACE); refreshBtn.setBase(ui::SURFACE);
    permitBtn.tag = 3; refreshBtn.tag = 4;
    permitBtn.setAction(clicked); refreshBtn.setAction(clicked);
    add(permitBtn); add(refreshBtn);
    footnote.setup(kit::SUB, kit::LEFT, 25, 428, 400, 32, ui::MUTED);
    footnote.set("Remote sense switches on only after the\nsense test passes.");
    add(footnote);

    static const char* const names[] = { "MOSFET", "Ambient", "Bleeder", "LDO/PCB" };
    for (int i = 0; i < 4; ++i) {
        const int x = 466 + i * 80;
        tempValue[i].setup(kit::M14, kit::CENTER, x, 140, 66, 20, ui::TEXT);
        tempLevel[i].setup(x + 21, 166, 24, 140);
        tempName[i].setup(kit::SUB, kit::CENTER, x - 4, 316, 74, 16, ui::MUTED);
        tempName[i].set(names[i]);
        add(tempValue[i]); add(tempLevel[i]); add(tempName[i]);
    }
    fanIcon.setup(462, 382, 52, 72, "", kit::Btn::GHOST);
    fanIcon.setBase(ui::SURFACE); fanIcon.setIcon(kit::ICON_FAN); fanIcon.setTouchable(false);
    fanIcon.setInk(ui::ACCENT); fanIcon.setIconSize(24);
    add(fanIcon);
    fanTitle.setup(kit::CAP, kit::LEFT, 524, 384, 200, 14, ui::MUTED);
    fanTitle.set("FAN");
    fanValue.setup(kit::M20, kit::M12, kit::LEFT, 523, 400, 250, 28, ui::TEXT);
    fanSub.setup(kit::SUB, kit::LEFT, 524, 430, 250, 16, ui::MUTED);
    add(fanTitle); add(fanValue); add(fanSub);
    KIT_PAGE_HEADER(FrontendApplication::SENSE);
}

void ScreenPowerPathView::onClick(const kit::Btn& b)
{
    switch (b.tag) {
    case 1: if (remoteOn) toggleSense(); break;
    case 2: if (!remoteOn) toggleSense(); break;
    case 3: togglePermit(); break;
    case 4: refreshTelemetry(); break;
    }
}

void ScreenPowerPathView::handleTickEvent()
{
    if (kit::app()->isScreenTransitionActive()) return;
    if (noticeTicks) --noticeTicks;
    if (++divider >= 4) { divider = 0; refresh(); }
}
void ScreenPowerPathView::notify(const char* text) { snprintf(notice, sizeof(notice), "%s", text); noticeTicks = 180; refresh(); }
void ScreenPowerPathView::allOff() { psu_app_power_shutdown(); notify("POWER OFF requested - BMS shutdown / wake with TS2"); }
void ScreenPowerPathView::togglePermit() { TelemetryRecord r(G4_RECORD_T); int64_t v; if (!r.get("permit", v)) { notify("No fresh permit state"); return; } notify(g4_permit(psu_g4(), v == 0, 0) ? "Permit change requested; waiting for telemetry" : "Request blocked"); }
void ScreenPowerPathView::toggleSense() { TelemetryRecord r(G4_RECORD_AUX); int64_t v; if (!r.get("sense_flags", v)) { notify("No fresh sense state"); return; } notify(g4_remote(psu_g4(), (v & 2) == 0, 0) ? "Sense change requested; waiting for telemetry" : "Request blocked"); }
void ScreenPowerPathView::refreshTelemetry() { notify(g4_simple(psu_g4(), "STATUS", 0) ? "PING sent - telemetry streams automatically" : "Command queue full"); }

void ScreenPowerPathView::refresh()
{
    TelemetryRecord r(G4_RECORD_T), aux(G4_RECORD_AUX);
    char b[64], rows[400];
    int64_t flags = 0, code = 0, local = 0, plus = 0, minus = 0, v = 0;
    const bool haveFlags = aux.get("sense_flags", flags);
    remoteOn = haveFlags && (flags & 1);
    const bool requested = haveFlags && (flags & 2);
    modeLocal.setSelected(haveFlags && !remoteOn && !requested);
    modeRemote.setSelected(remoteOn || requested);
    modeLocal.setEnabled(haveFlags); modeRemote.setEnabled(haveFlags);
    snprintf(b, sizeof(b), "relay K1 %s \xC2\xB7 fault latch %s", !haveFlags ? "--" : remoteOn ? "remote" : "local", !haveFlags ? "--" : (flags & 4) ? "SET" : "clear");
    senseCard.setNote(b, haveFlags && (flags & 4) ? ui::NEGATIVE : ui::MUTED);

    if (aux.get("local_mv", local)) { kit::milli(b, sizeof(b), local, "V"); localBox.setDetail(b, kit::M20); } else localBox.setDetail("-- V", kit::M20);
    const bool haveRemote = aux.get("remote_p_mv", plus) && aux.get("remote_n_mv", minus);
    if (haveRemote && remoteOn) { kit::milli(b, sizeof(b), plus - minus, "V"); loadBox.setDetail(b, kit::M20); } else loadBox.setDetail("-- V", kit::M20);

    rows[0] = 0;
    if (haveRemote) { char p[24], m[24]; kit::milli(p, sizeof(p), plus, "", 3); kit::milli(m, sizeof(m), minus, "V", 3); snprintf(b, sizeof(b), "%s / %s", p, m); }
    else strcpy(b, "-- / -- V");
    TelemetryRecord::append(rows, sizeof(rows), "Remote + / -", b);
    if (haveRemote && aux.get("local_mv", local)) snprintf(b, sizeof(b), "%ld / %ld mV", (long)(local - plus), (long)minus); else strcpy(b, "--");
    TelemetryRecord::append(rows, sizeof(rows), "Lead drop P / N", b);
    const bool haveCode = aux.get("sense_code", code);
    const err::Entry* senseEntry = haveCode ? err::find(err::E_SENSE, (uint32_t)code) : 0;
    TelemetryRecord::append(rows, sizeof(rows), "Sense self-test", !haveCode ? "--" : code == 0 ? "PASSED" : senseEntry ? senseEntry->symbol : "UNKNOWN");
    TelemetryRecord::append(rows, sizeof(rows), "Remote request", haveFlags ? (requested ? "YES" : "NO") : "--");
    r.flag(rows, sizeof(rows), "LDO permit", "permit", "ALLOWED", "BLOCKED");
    r.state(b, sizeof(b), "run", "RUNNING", "STOPPED");
    TelemetryRecord::append(rows, sizeof(rows), "PSU path", b);
    senseRows.set(rows, aux.fresh);
    const bool permit = r.is("permit", 1);
    permitBtn.setLabel(!r.fresh ? "LDO PERMIT: --" : permit ? "LDO PERMIT: ON" : "LDO PERMIT: OFF");
    permitBtn.setEnabled(r.fresh);

    static const char* const keys[] = { "t1", "t2", "t3", "t4" };
    for (int i = 0; i < 4; ++i) {
        if (aux.get(keys[i], v)) {
            const int64_t a = v < 0 ? -v : v;
            snprintf(b, sizeof(b), "%s%ld.%ld\xC2\xB0", v < 0 ? "-" : "", (long)(a / 10), (long)(a % 10));
            tempValue[i].set(b, ui::TEXT);
            const ui::Role role = v >= 700 ? ui::NEGATIVE : v >= 500 ? ui::CAUTION : ui::POSITIVE;
            tempLevel[i].set((int)(v * 1000 / 900), role);
        } else { tempValue[i].set("--", ui::MUTED); tempLevel[i].set(0, ui::MUTED); }
    }
    int64_t rpm = 0, pwm = 0;
    if (aux.get("fan_rpm", rpm)) { snprintf(b, sizeof(b), "%ld RPM", (long)rpm); fanValue.set(b, ui::TEXT); } else fanValue.set("-- RPM", ui::MUTED);
    if (aux.get("fan", pwm)) snprintf(b, sizeof(b), "PWM %ld %% \xC2\xB7 auto", (long)pwm); else strcpy(b, "PWM --");
    fanSub.set(b);

    if (noticeTicks) header.setSub(notice, ui::TEXT);
    else if (aux.get("sense_code", code)) {
        const err::Entry* e = err::find(err::E_SENSE, (uint32_t)code);
        header.setSub(code == 1 ? "Low output: sense wiring not evaluated yet" : e ? e->text : "Sense test failed - inspect remote wiring", code > 1 ? ui::NEGATIVE : ui::MUTED);
    }
    else header.setSub(aux.record.valid ? "Sense telemetry stale" : "Waiting for sense telemetry", ui::CAUTION);
}
