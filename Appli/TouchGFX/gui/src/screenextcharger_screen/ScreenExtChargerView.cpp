#include <gui/screenextcharger_screen/ScreenExtChargerView.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace {
FrontendApplication* app() { return static_cast<FrontendApplication*>(touchgfx::Application::getInstance()); }
const char* const chemNames[] = { "Li-ion", "LiHV", "LiFePO\xE2\x82\x84", "LTO", "Lead", "NiMH" };
const char* const keyLabels[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "DEL", "CLR", "APPLY" };
void milli(char* out, size_t n, uint32_t v, const char* unit)
{
    snprintf(out, n, "%lu.%03lu %s", (unsigned long)(v / 1000), (unsigned long)(v % 1000), unit);
}
// 0 = CC (incl. validation / precharge), 1 = CV, 2 = done, -1 = idle / stopped.
int phaseOf(PsuChgState s)
{
    switch (s) {
    case CHG_VALIDATE: case CHG_WAIT: case CHG_STARTING: case CHG_PRECHARGE: case CHG_CC: return 0;
    case CHG_CV: case CHG_ABSORPTION: case CHG_FLOAT: case CHG_TERMINATING: return 1;
    case CHG_COMPLETE: return 2;
    default: return -1;
    }
}
}

ScreenExtChargerView::ScreenExtChargerView()
    : field(NONE), divider(0), polarity(false), clicked(this, &ScreenExtChargerView::onClick)
{
    psu_editor_clear(&editor);
}

void ScreenExtChargerView::setupScreen()
{
    ScreenExtChargerViewBase::setupScreen();
    PsuCharger* c = psu_charger();
    // The polarity confirmation belongs to this visit of the screen.
    if (!c->running && c->state != CHG_FAULT && c->state != CHG_ABORTED) c->reason[0] = 0;
    layout();
    setupTheme();
    refresh();
}

void ScreenExtChargerView::layout()
{
    app()->setSwipePage(FrontendApplication::CHARGER);
    kit::clearNoSwipe();
    kit::hideAll(getRootContainer());
    LabBackground.setVisible(true);

    setupCard.setup(12, 58, 400, 410);
    sessionCard.setup(422, 58, 366, 410);
    add(setupCard); add(sessionCard);

    // ---- left: setup steps
    const char* captions[] = { "CHEMISTRY", "PACK", "SAFETY" };
    const int stepY[] = { 70, 150, 308 };
    for (int i = 0; i < 3; ++i) {
        char n[2] = { char('1' + i), 0 };
        step[i].setup(24, stepY[i], 22, 22, n, kit::Btn::PRIMARY, kit::CAP);
        step[i].setBase(ui::SURFACE); step[i].setTouchable(false);
        stepCaption[i].setup(kit::CAP, kit::LEFT, 54, stepY[i] + 4, 200, 16, ui::MUTED);
        stepCaption[i].set(captions[i]);
        add(step[i]); add(stepCaption[i]);
    }
    for (int i = 0; i < CHEMS; ++i) {
        chem[i].setup(24 + i * 63, 98, 60, 40, chemNames[i], kit::Btn::SEGMENT, kit::BODYM);
        chem[i].setBase(ui::SURFACE); chem[i].tag = 100 + i; chem[i].setAction(clicked);
        add(chem[i]);
    }
    cellsBox.setup(24, 176, 184, 60, ui::RAISED, ui::BORDER, ui::SURFACE, 8);
    add(cellsBox);
    cellsMinus.setup(30, 183, 44, 46, "", kit::Btn::NORMAL, kit::HEAD); cellsMinus.setIcon(kit::ICON_MINUS);
    cellsPlus.setup(158, 183, 44, 46, "", kit::Btn::NORMAL, kit::HEAD); cellsPlus.setIcon(kit::ICON_PLUS);
    cellsMinus.setBase(ui::RAISED); cellsPlus.setBase(ui::RAISED);
    cellsMinus.tag = 200; cellsPlus.tag = 201;
    cellsMinus.setAction(clicked); cellsPlus.setAction(clicked);
    cellsValue.setup(kit::M20, kit::CENTER, 74, 184, 84, 26, ui::TEXT);
    cellsSub.setup(kit::SUB, kit::CENTER, 74, 210, 84, 16, ui::MUTED);
    cellsSub.set("in series");
    add(cellsMinus); add(cellsPlus); add(cellsValue); add(cellsSub);

    target.setup(216, 176, 184, 60, "Target pack voltage", kit::Btn::FIELD, kit::SUB);
    current.setup(24, 244, 184, 56, "Charge current", kit::Btn::FIELD, kit::SUB);
    stop.setup(216, 244, 184, 56, "Stop below", kit::Btn::FIELD, kit::SUB);
    kit::Btn* fields[] = { &target, &current, &stop };
    for (int i = 0; i < 3; ++i) {
        fields[i]->setBase(ui::SURFACE); fields[i]->tag = 300 + i; fields[i]->setAction(clicked);
        fields[i]->setInk(i == 0 ? ui::VOLT : ui::AMP);
        add(*fields[i]);
    }
    polarityBtn.setup(24, 334, 376, 58, "Polarity", kit::Btn::FIELD, kit::SUB);
    polarityBtn.setBase(ui::SURFACE); polarityBtn.tag = 400; polarityBtn.setAction(clicked);
    add(polarityBtn);
    note.setup(kit::BODY, kit::LEFT, 26, 402, 372, 56, ui::MUTED);
    add(note);

    // ---- right: session
    const char* phases[] = { "CC \xC2\xB7 CURRENT", "CV \xC2\xB7 VOLTAGE", "DONE" };
    for (int i = 0; i < 3; ++i) {
        phase[i].setup(434 + i * 116, 70, 110, 32, phases[i], kit::Btn::SEGMENT, kit::CAP);
        phase[i].setBase(ui::SURFACE); phase[i].setTouchable(false);
        add(phase[i]);
    }
    liveV.setup(kit::M34, kit::LEFT, 434, 110, 170, 44, ui::VOLT);
    liveA.setup(kit::M34, kit::LEFT, 610, 110, 170, 44, ui::AMP);
    add(liveV); add(liveA);
    const char* stats[] = { "TIME", "CHARGED", "ENERGY" };
    for (int i = 0; i < 3; ++i) {
        statCaption[i].setup(kit::CAP, kit::LEFT, 436 + i * 116, 162, 110, 14, ui::MUTED);
        statCaption[i].set(stats[i]);
        statValue[i].setup(kit::M16, kit::LEFT, 436 + i * 116, 178, 112, 22, i == 2 ? ui::WATT : ui::TEXT);
        add(statCaption[i]); add(statValue[i]);
    }
    plot.setup(434, 210, 342, 128);
    add(plot);
    plotEmpty.setup(kit::SUB, kit::CENTER, 434, 248, 342, 16, ui::MUTED);
    plotEmpty.set("Chart starts with the charge session");
    add(plotEmpty);
    footnote.setup(kit::SUB, kit::LEFT, 436, 346, 340, 36, ui::MUTED);
    add(footnote);
    start.setup(434, 408, 167, 48, "START", kit::Btn::POSITIVE_FILL, kit::LABEL);
    stopBtn.setup(609, 408, 167, 48, "STOP", kit::Btn::DANGER, kit::LABEL);
    start.setBase(ui::SURFACE); stopBtn.setBase(ui::SURFACE);
    start.tag = 500; stopBtn.tag = 501;
    start.setAction(clicked); stopBtn.setAction(clicked);
    add(start); add(stopBtn);

    // ---- right: keypad (shown while a field is edited)
    editCaption.setup(kit::CAP, kit::LEFT, 436, 70, 330, 14, ui::ACCENT);
    editValue.setup(kit::M34, kit::LEFT, 434, 86, 340, 44, ui::TEXT);
    editHint.setup(kit::SUB, kit::LEFT, 436, 130, 340, 16, ui::MUTED);
    add(editCaption); add(editValue); add(editHint);
    for (int i = 0; i < KEYS; ++i) {
        int x, y = 152 + (i / 3) * 62, w = 108;
        if (i < 12) x = 434 + (i % 3) * 117;
        else if (i == 12) { x = 434; y = 152 + 4 * 62; }
        else { x = 551; y = 152 + 4 * 62; w = 225; }
        keys[i].setup(x, y, w, 56, keyLabels[i], i == 13 ? kit::Btn::PRIMARY : kit::Btn::KEY, i < 11 ? kit::M20 : kit::BODYM);
        keys[i].setBase(ui::SURFACE); keys[i].tag = 600 + i; keys[i].setAction(clicked);
        add(keys[i]);
    }
    keys[11].setLabel(""); keys[11].setIcon(kit::ICON_DEL);

    header.setup(FrontendApplication::pageInfo(FrontendApplication::CHARGER));
    add(header);
    showEditor(false);
}

void ScreenExtChargerView::setupTheme()
{
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this);
    theme.box(LabBackground, ui::BACKGROUND);
    theme.apply();
}

void ScreenExtChargerView::handleTickEvent()
{
    if (app()->isScreenTransitionActive()) return;
    if (++divider >= 6) { divider = 0; refresh(); }
}

void ScreenExtChargerView::showEditor(bool on)
{
    touchgfx::Drawable* session[] = { &phase[0], &phase[1], &phase[2], &liveV, &liveA,
        &statCaption[0], &statCaption[1], &statCaption[2], &statValue[0], &statValue[1], &statValue[2],
        &plot, &plotEmpty, &footnote, &start, &stopBtn };
    for (touchgfx::Drawable* d : session) if (d->isVisible() == on) { d->setVisible(!on); d->invalidate(); }
    touchgfx::Drawable* editorParts[] = { &editCaption, &editValue, &editHint };
    for (touchgfx::Drawable* d : editorParts) if (d->isVisible() != on) { d->setVisible(on); d->invalidate(); }
    for (int i = 0; i < KEYS; ++i) if (keys[i].isVisible() != on) { keys[i].setVisible(on); keys[i].invalidate(); }
}

void ScreenExtChargerView::onClick(const kit::Btn& b)
{
    PsuCharger* c = psu_charger();
    const int t = b.tag;
    if (t >= 100 && t < 100 + CHEMS) {
        if (c->running) return;
        psu_chg_profile_defaults(&c->profile, (uint8_t)(t - 100));
        polarity = false; field = NONE; showEditor(false);
    } else if (t == 200 || t == 201) {
        setCells(t == 200 ? -1 : 1);
    } else if (t >= 300 && t <= 302) {
        edit(t - 300);
    } else if (t == 400) {
        if (polarity) { polarity = false; snprintf(c->reason, sizeof(c->reason), "Polarity check cleared"); }
        else confirmPolarity();
    } else if (t == 500) {
        startCharge();
    } else if (t == 501) {
        stopCharge();
    } else if (t >= 600) {
        const int k = t - 600;
        if (k <= 10) key(keyLabels[k][0]);
        else if (k == 11) psu_editor_backspace(&editor);
        else if (k == 12) psu_editor_clear(&editor);
        else apply();
    }
    refresh();
}

void ScreenExtChargerView::setCells(int delta)
{
    PsuCharger* c = psu_charger();
    if (c->running) return;
    PsuChgProfile& p = c->profile;
    int n = (int)p.cells + delta;
    const int max = psu_chg_max_cells(&p);
    if (n < 1) n = 1;
    if (n > max) n = max;
    if (n != p.cells) { p.cells = (uint8_t)n; p.target_pack_mv = 0; polarity = false; }
}

void ScreenExtChargerView::edit(int f)
{
    PsuCharger* c = psu_charger();
    if (c->running) return;
    if (field == f) { field = NONE; showEditor(false); return; }
    field = f;
    const PsuChgProfile& p = c->profile;
    psu_editor_load_milli(&editor, f == TARGET ? psu_chg_target_mv(&p) : f == CURRENT ? p.cc_ma : p.term_ma, 3);
    showEditor(true);
}

void ScreenExtChargerView::key(char k)
{
    if (psu_charger()->running || field == NONE) return;
    psu_editor_key(&editor, k);
}

void ScreenExtChargerView::apply()
{
    PsuCharger* c = psu_charger();
    if (c->running || field == NONE) return;
    uint32_t v;
    if (!psu_editor_parse_milli(&editor, &v)) { psu_editor_clear(&editor); return; }
    PsuChgProfile& p = c->profile;
    if (field == TARGET && !psu_chg_set_target(&p, v)) {
        snprintf(c->reason, sizeof(c->reason), "Target must be %lu-%lu mV",
                 (unsigned long)(p.cells * p.precharge_mv_cell), (unsigned long)(p.cells * p.cv_mv_cell));
        return;
    }
    if (field == CURRENT) { if (v > 5000) v = 5000; if (v < 1) v = 1; p.cc_ma = v; if (p.term_ma > v) p.term_ma = v; }
    if (field == STOP) { if (v > p.cc_ma) v = p.cc_ma; if (v < 1) v = 1; p.term_ma = v; }
    if (p.precharge_ma > p.cc_ma) p.precharge_ma = p.cc_ma;
    polarity = false;
    field = NONE;
    showEditor(false);
}

void ScreenExtChargerView::confirmPolarity()
{
    if (psu_charger()->running || field != NONE) return;
    polarity = true;
    snprintf(psu_charger()->reason, sizeof(psu_charger()->reason), "Polarity confirmed");
}

void ScreenExtChargerView::startCharge()
{
    PsuSnapshot live; psu_snapshot(&live);
    PsuCharger* c = psu_charger();
    if (field != NONE || !polarity || psu_seq_edit_locked(psu_sequencer()) || live.shutdown_pending) {
        snprintf(c->reason, sizeof(c->reason), !polarity ? "Confirm polarity first" : "Stop other automation first");
        return;
    }
    c->profile.confirmed = 1; c->profile.polarity_checked = 1;
    psu_app_start_charging();
}

void ScreenExtChargerView::stopCharge()
{
    psu_app_shutdown();
    snprintf(psu_charger()->reason, sizeof(psu_charger()->reason), "Operator stop");
    polarity = false;
}

void ScreenExtChargerView::allOff()
{
    psu_app_power_shutdown();
    polarity = false;
    refresh();
}

void ScreenExtChargerView::refresh()
{
    PsuCharger* c = psu_charger();
    const PsuChgProfile& p = c->profile;
    PsuSnapshot live; psu_snapshot(&live);
    char b[96];
    if (c->running && field != NONE) { field = NONE; showEditor(false); }
    const bool locked = c->running;
    const bool fault = c->state == CHG_FAULT || c->state == CHG_ABORTED;

    snprintf(b, sizeof(b), "%s \xC2\xB7 %s", psu_chg_state_name(c->state), (c->reason[0] && strcmp(c->reason, psu_chg_state_name(c->state))) ? c->reason : "charges a pack on the output terminals");
    header.setSub(b, fault ? ui::NEGATIVE : ui::MUTED);

    for (int i = 0; i < CHEMS; ++i) { chem[i].setSelected(p.chemistry == i); chem[i].setEnabled(!locked); }
    snprintf(b, sizeof(b), "%uS", (unsigned)p.cells);
    cellsValue.set(b);
    cellsMinus.setEnabled(!locked && p.cells > 1);
    cellsPlus.setEnabled(!locked && p.cells < psu_chg_max_cells(&p));

    const uint32_t targetMv = psu_chg_target_mv(&p);
    milli(b, sizeof(b), targetMv, "V"); target.setDetail(b, kit::M20);
    milli(b, sizeof(b), p.cc_ma, "A"); current.setDetail(b, kit::M20);
    milli(b, sizeof(b), p.term_ma, "A"); stop.setDetail(b, kit::M20);
    target.setSelected(field == TARGET); current.setSelected(field == CURRENT); stop.setSelected(field == STOP);
    target.setEnabled(!locked); current.setEnabled(!locked); stop.setEnabled(!locked);

    const bool voltsFresh = live.g0_connected && !live.g0_stale;
    polarityBtn.setEnabled(!locked && field == NONE);
    polarityBtn.setSelected(polarity || locked);
    polarityBtn.setLabel(polarity || locked ? "Polarity confirmed" : "Confirm polarity");
    if (voltsFresh) {
        char v[24]; milli(v, sizeof(v), live.vout_mv, "V");
        snprintf(b, sizeof(b), "pack + on red terminal \xC2\xB7 %s", v);
    } else snprintf(b, sizeof(b), "pack + on red terminal");
    polarityBtn.setDetail(b, kit::BODY);
    step[2].setLabel(polarity || locked ? "" : "3");
    step[2].setIcon(polarity || locked ? kit::ICON_CHECK : kit::ICON_NONE);
    step[2].setStyle(polarity || locked ? kit::Btn::POSITIVE_FILL : kit::Btn::PRIMARY);

    const char* hint;
    if (field != NONE) hint = "Type the value, APPLY to save.\nTap the field again to cancel.";
    else if (locked) hint = "Charging. Settings are locked\nuntil the session stops.";
    else if (!polarity) hint = "Check the wiring, then confirm\npolarity to enable START.";
    else hint = "Ready. START charges with the\nprofile shown above.";
    note.set(hint, fault ? ui::NEGATIVE : ui::MUTED);

    if (field != NONE) {
        const char* captions[] = { "TARGET PACK VOLTAGE / V", "CHARGE CURRENT / A", "STOP BELOW CURRENT / A" };
        editCaption.set(captions[field]);
        editValue.set(editor.text[0] ? editor.text : "0", field == TARGET ? ui::VOLT : ui::AMP);
        if (field == TARGET)
            snprintf(b, sizeof(b), "range %lu.%02lu - %lu.%02lu V", (unsigned long)(p.cells * p.precharge_mv_cell / 1000), (unsigned long)(p.cells * p.precharge_mv_cell % 1000 / 10),
                     (unsigned long)(p.cells * p.cv_mv_cell / 1000), (unsigned long)(p.cells * p.cv_mv_cell % 1000 / 10));
        else snprintf(b, sizeof(b), field == CURRENT ? "max 5.000 A" : "at most the charge current");
        editHint.set(b);
        keys[9].setEnabled(true);
        ui::ThemeScreen::get().sync();
        return;
    }

    const int ph = phaseOf(c->state);
    for (int i = 0; i < 3; ++i) {
        phase[i].setSelected(ph == i);
        phase[i].setInk(ph > i ? ui::POSITIVE : -1);
    }
    if (voltsFresh) milli(b, sizeof(b), live.vout_mv, "V"); else strcpy(b, "-.--- V");
    liveV.set(b);
    if (live.current_valid) {
        const long long ua = llabs(live.display_current_ua);
        snprintf(b, sizeof(b), "%s%lu.%03lu A", live.display_current_ua < 0 ? "-" : "", (unsigned long)(ua / 1000000), (unsigned long)(ua / 1000 % 1000));
    } else strcpy(b, "-.--- A");
    liveA.set(b);
    snprintf(b, sizeof(b), "%02lu:%02lu:%02lu", (unsigned long)(c->elapsed_ms / 3600000), (unsigned long)(c->elapsed_ms / 60000 % 60), (unsigned long)(c->elapsed_ms / 1000 % 60));
    statValue[0].set(b);
    snprintf(b, sizeof(b), "%lu mAh", (unsigned long)c->delivered_mah);
    statValue[1].set(b);
    snprintf(b, sizeof(b), "%lu.%02lu Wh", (unsigned long)(c->delivered_mwh / 1000), (unsigned long)(c->delivered_mwh % 1000 / 10));
    statValue[2].set(b);

    // Profile chart: voltage (solid) rising towards the target, current (dashed).
    static int16_t v[128], a[128];
    const int n = c->trace_count > 128 ? 128 : c->trace_count;
    int16_t vlo = (int16_t)(targetMv * 7 / 10), vhi = (int16_t)(targetMv + targetMv / 20);
    for (int i = 0; i < n; ++i) {
        v[i] = (int16_t)(c->trace_mv[i] > 32000 ? 32000 : c->trace_mv[i]);
        a[i] = (int16_t)(c->trace_ma[i] > 32000 ? 32000 : c->trace_ma[i]);
        if (v[i] < vlo) vlo = v[i];
    }
    plot.setSeries(0, v, n, vlo, vhi, ui::VOLT);
    if (plotEmpty.isVisible() != (n < 2)) { plotEmpty.setVisible(n < 2); plotEmpty.invalidate(); }
    plot.setSeries(1, a, n, 0, (int16_t)(p.cc_ma + p.cc_ma / 4 + 1), ui::AMP, true);
    {
        char tv[24], ti[24]; milli(tv, sizeof(tv), targetMv, "V"); milli(ti, sizeof(ti), p.term_ma, "A");
        snprintf(b, sizeof(b), "Switches to CV at %s\nstops below %s \xC2\xB7 %s %uS", tv, ti, chemNames[p.chemistry < CHEMS ? p.chemistry : 0], (unsigned)p.cells);
        footnote.set(b);
    }
    start.setEnabled(!locked && polarity);
    stopBtn.setEnabled(locked || live.output_requested || live.output_confirmed || live.shutdown_pending);
    ui::ThemeScreen::get().sync();
}
