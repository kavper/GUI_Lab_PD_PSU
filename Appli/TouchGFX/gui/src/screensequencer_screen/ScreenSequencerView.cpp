#include <gui/screensequencer_screen/ScreenSequencerView.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/UiTheme.hpp>
#include <stdio.h>
#include <string.h>

namespace {
FrontendApplication* app() { return static_cast<FrontendApplication*>(touchgfx::Application::getInstance()); }
const char* const keyLabels[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "DEL", "CLR", "APPLY" };
// Table columns -> editor fields (column 0 is the step number).
const uint8_t columnField[] = { PSU_SEQ_FIELD_NONE, PSU_SEQ_FIELD_V, PSU_SEQ_FIELD_I, PSU_SEQ_FIELD_S, PSU_SEQ_FIELD_T };
const int columnX[] = { 22, 62, 172, 282, 412 };
const int columnW[] = { 38, 108, 108, 128, 144 };
const int ROW_Y = 218, ROW_H = 34;
void clock(char* out, size_t n, uint32_t ms)
{
    const uint32_t s = ms / 1000;
    if (s >= 3600) snprintf(out, n, "%lu:%02lu:%02lu", (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60), (unsigned long)(s % 60));
    else snprintf(out, n, "%02lu:%02lu", (unsigned long)(s / 60), (unsigned long)(s % 60));
}
}

ScreenSequencerView::ScreenSequencerView()
    : divider(0), first(0), cycleEditing(false), listDragging(false), dragPixels(0),
      clicked(this, &ScreenSequencerView::onClick)
{
    psu_seq_edit_init(&edit);
    psu_editor_clear(&cycleEditor);
}

void ScreenSequencerView::setupScreen()
{
    ScreenSequencerViewBase::setupScreen();
    psu_app_ensure();
    layout();
    setupTheme();
    refresh();
}

void ScreenSequencerView::setupTheme()
{
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this);
    theme.box(LabBackground, ui::BACKGROUND);
    theme.apply();
}

void ScreenSequencerView::layout()
{
    app()->setSwipePage(FrontendApplication::SEQUENCER);
    kit::clearNoSwipe();
    kit::hideAll(getRootContainer());
    LabBackground.setVisible(true);

    profileCard.setup(12, 58, 556, 124);
    stepsCard.setup(12, 192, 556, 276);
    runCard.setup(578, 58, 210, 410);
    add(profileCard); add(stepsCard); add(runCard);

    // ---- profile
    profileCaption.setup(kit::CAP, kit::LEFT, 26, 68, 120, 14, ui::MUTED);
    profileCaption.set("PROFILE");
    legend.setup(kit::SUB, kit::RIGHT, 150, 66, 404, 16, ui::MUTED);
    plot.setup(24, 86, 532, 88);
    plot.setGrid(2);
    add(profileCaption); add(legend); add(plot);

    // ---- step table
    const char* heads[] = { "#", "VOLTAGE", "CURRENT", "RAMP", "HOLD" };
    for (int c = 0; c < COLS; ++c) {
        tableHead[c].setup(kit::CAP, kit::CENTER, columnX[c], 200, columnW[c], 14, ui::MUTED);
        tableHead[c].set(heads[c]);
        add(tableHead[c]);
    }
    listRange.setup(kit::CAP, kit::RIGHT, 440, 200, 116, 14, ui::MUTED);
    rowMark.setup(18, ROW_Y, 544, ROW_H - 2, ui::RAISED, ui::ACCENT, ui::SURFACE, 7);
    add(rowMark);
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c) {
            kit::Btn& b = cell[r][c];
            b.setup(columnX[c], ROW_Y + r * ROW_H + 2, columnW[c], ROW_H - 6, "", kit::Btn::GHOST, c == 0 ? kit::M14 : kit::M16);
            b.setBase(ui::SURFACE);
            b.tag = 100 + r * 8 + c;
            b.setAction(clicked);
            add(b);
        }
    const char* actions[] = { "+ add step (up to 12)", "\xE2\x86\x91 Move", "\xE2\x86\x93 Move", "Duplicate", "Delete" };
    for (int i = 0; i < ACTIONS; ++i) {
        if (i == 0) action[i].setup(20, ROW_Y, 540, ROW_H - 4, actions[i], kit::Btn::FIELD, kit::SUB);
        else action[i].setup(24 + (i - 1) * 134, 414, 128, 44, actions[i], kit::Btn::NORMAL, kit::BODYM);
        action[i].setBase(ui::SURFACE);
        action[i].tag = 200 + i;
        action[i].setAction(clicked);
        add(action[i]);
    }
    action[0].setInk(ui::MUTED);
    action[4].setInk(ui::NEGATIVE);
    add(listRange);

    // ---- run card
    runCaption.setup(kit::CAP, kit::LEFT, 590, 70, 80, 14, ui::MUTED);
    runCaption.set("RUN");
    runChip.setXY(680, 66);
    runTime.setup(kit::M26, kit::LEFT, 588, 88, 190, 34, ui::TEXT);
    runTotal.setup(kit::SUB, kit::LEFT, 590, 122, 186, 16, ui::MUTED);
    progress.setup(590, 144, 186, 8, ui::POSITIVE);
    runInfo.setup(kit::SUB, kit::LEFT, 590, 160, 186, 34, ui::MUTED);
    repeatCaption.setup(kit::CAP, kit::LEFT, 590, 206, 120, 14, ui::MUTED);
    repeatCaption.set("REPEAT");
    add(runCaption); add(runChip); add(runTime); add(runTotal); add(progress); add(runInfo); add(repeatCaption);
    const char* modes[] = { "ONCE", "N \xC3\x97", "LOOP" };
    for (int i = 0; i < 3; ++i) {
        mode[i].setup(590 + i * 63, 224, 60, 38, modes[i], kit::Btn::SEGMENT, kit::BODYM);
        mode[i].setBase(ui::SURFACE); mode[i].tag = 300 + i; mode[i].setAction(clicked);
        add(mode[i]);
    }
    cycleLess.setup(590, 270, 50, 44, "", kit::Btn::NORMAL, kit::HEAD); cycleLess.setIcon(kit::ICON_MINUS);
    cycleCount.setup(644, 270, 78, 44, "", kit::Btn::FIELD, kit::M16);
    cycleMore.setup(726, 270, 50, 44, "", kit::Btn::NORMAL, kit::HEAD); cycleMore.setIcon(kit::ICON_PLUS);
    kit::Btn* cyc[] = { &cycleLess, &cycleCount, &cycleMore };
    for (int i = 0; i < 3; ++i) { cyc[i]->setBase(ui::SURFACE); cyc[i]->tag = 310 + i; cyc[i]->setAction(clicked); add(*cyc[i]); }
    runBtn.setup(590, 350, 186, 52, "RUN", kit::Btn::POSITIVE_FILL, kit::LABEL);
    stopBtn.setup(590, 408, 186, 50, "STOP", kit::Btn::DANGER, kit::LABEL);
    runBtn.setBase(ui::SURFACE); stopBtn.setBase(ui::SURFACE);
    runBtn.tag = 400; stopBtn.tag = 401;
    runBtn.setAction(clicked); stopBtn.setAction(clicked);
    add(runBtn); add(stopBtn);

    // ---- keypad (replaces the run card while editing)
    editCaption.setup(kit::CAP, kit::LEFT, 590, 70, 186, 14, ui::ACCENT);
    editValue.setup(kit::M26, kit::LEFT, 588, 88, 190, 34, ui::TEXT);
    editHint.setup(kit::SUB, kit::LEFT, 590, 124, 186, 16, ui::MUTED);
    add(editCaption); add(editValue); add(editHint);
    for (int i = 0; i < KEYS; ++i) {
        int x, y = 148 + (i / 3) * 62, w = 58;
        if (i < 12) x = 590 + (i % 3) * 64;
        else if (i == 12) { x = 590; y = 148 + 4 * 62; }
        else { x = 654; y = 148 + 4 * 62; w = 122; }
        keys[i].setup(x, y, w, 56, keyLabels[i], i == 13 ? kit::Btn::PRIMARY : kit::Btn::KEY, i < 11 ? kit::M20 : kit::BODYM);
        keys[i].setBase(ui::SURFACE); keys[i].tag = 500 + i; keys[i].setAction(clicked);
        add(keys[i]);
    }
    keys[11].setLabel(""); keys[11].setIcon(kit::ICON_DEL);

    header.setup(FrontendApplication::pageInfo(FrontendApplication::SEQUENCER));
    add(header);
    kit::addNoSwipe(touchgfx::Rect(12, ROW_Y, 556, ROWS * ROW_H));
    showEditor(false);
}

void ScreenSequencerView::showEditor(bool on)
{
    touchgfx::Drawable* runParts[] = { &runCaption, &runChip, &runTime, &runTotal, &progress, &runInfo, &repeatCaption,
        &mode[0], &mode[1], &mode[2], &cycleLess, &cycleCount, &cycleMore, &runBtn, &stopBtn };
    for (touchgfx::Drawable* d : runParts) if (d->isVisible() == on) { d->setVisible(!on); d->invalidate(); }
    touchgfx::Drawable* editParts[] = { &editCaption, &editValue, &editHint };
    for (touchgfx::Drawable* d : editParts) if (d->isVisible() != on) { d->setVisible(on); d->invalidate(); }
    for (int i = 0; i < KEYS; ++i) if (keys[i].isVisible() != on) { keys[i].setVisible(on); keys[i].invalidate(); }
}

void ScreenSequencerView::handleTickEvent()
{
    if (app()->isScreenTransitionActive()) return;
    if (++divider < 8) return;
    divider = 0;
    refresh();
}

uint32_t ScreenSequencerView::rampMs(uint32_t fromMv, const PsuSeqStep& s)
{
    if (s.slew_mv_per_s == 0) return 0;
    const uint32_t dv = s.voltage_mv > fromMv ? s.voltage_mv - fromMv : fromMv - s.voltage_mv;
    return (uint32_t)((uint64_t)dv * 1000 / s.slew_mv_per_s);
}

void ScreenSequencerView::refreshProfile(uint32_t& totalMs, uint32_t& elapsedMs)
{
    PsuSequencer* seq = psu_sequencer();
    uint32_t start[PSU_SEQ_MAX_STEPS], ramp[PSU_SEQ_MAX_STEPS], from[PSU_SEQ_MAX_STEPS];
    uint32_t t = 0, mv = 0, vmax = 1000, imax = 100;
    for (unsigned i = 0; i < seq->count; ++i) {
        const PsuSeqStep& s = seq->steps[i];
        start[i] = t; from[i] = mv;
        ramp[i] = s.enabled ? rampMs(mv, s) : 0;
        if (s.enabled) { t += ramp[i] + s.time_ms; mv = s.voltage_mv; }
        if (s.voltage_mv > vmax) vmax = s.voltage_mv;
        if (s.current_ma > imax) imax = s.current_ma;
    }
    totalMs = t;
    elapsedMs = 0;
    const bool active = seq->run == PSU_SEQ_RUN || seq->run == PSU_SEQ_PAUSE;
    if (active && seq->step_index < seq->count) {
        uint32_t inStep = psu_app_now() - seq->step_started_ms;
        const PsuSeqStep& s = seq->steps[seq->step_index];
        if (inStep > ramp[seq->step_index] + s.time_ms) inStep = ramp[seq->step_index] + s.time_ms;
        elapsedMs = start[seq->step_index] + inStep;
    } else if (seq->run == PSU_SEQ_DONE) elapsedMs = totalMs;

    // Sample both series; values in 10 mV / mA units to stay in int16.
    static int16_t v[POINTS], a[POINTS];
    for (int p = 0; p < POINTS; ++p) {
        const uint32_t at = totalMs ? (uint32_t)((uint64_t)totalMs * p / (POINTS - 1)) : 0;
        unsigned i = 0;
        while (i + 1 < seq->count && (!seq->steps[i].enabled || at >= start[i] + ramp[i] + seq->steps[i].time_ms)) ++i;
        const PsuSeqStep& s = seq->steps[i];
        uint32_t level = s.voltage_mv;
        if (at < start[i] + ramp[i] && ramp[i]) {
            const uint32_t k = at > start[i] ? at - start[i] : 0;
            level = (uint32_t)((int64_t)from[i] + ((int64_t)s.voltage_mv - (int64_t)from[i]) * (int64_t)k / (int64_t)ramp[i]);
        }
        v[p] = (int16_t)(level / 10);
        a[p] = (int16_t)(s.current_ma > 32000 ? 32000 : s.current_ma);
    }
    const int n = seq->count ? POINTS : 0;
    plot.setSeries(0, v, n, 0, (int16_t)(vmax / 10 + vmax / 100 + 1), ui::VOLT);
    plot.setSeries(1, a, n, 0, (int16_t)(imax + imax / 8 + 1), ui::AMP, true);
    plot.setCursor(active || seq->run == PSU_SEQ_DONE ? (int)((uint64_t)elapsedMs * (plot.getWidth() - 2) / (totalMs ? totalMs : 1)) : -1);

    char total[20], b[64];
    clock(total, sizeof(total), totalMs);
    snprintf(b, sizeof(b), "\xE2\x80\x93 voltage   - - current   total %s", total);
    legend.set(b);
}

void ScreenSequencerView::refresh()
{
    PsuSequencer* seq = psu_sequencer();
    const bool locked = psu_seq_edit_locked(seq) != 0;
    if (locked && editing()) { cycleEditing = false; edit.field = PSU_SEQ_FIELD_NONE; showEditor(false); }
    const uint8_t count = seq->count;
    uint8_t sel = count ? (seq->selected < count ? seq->selected : count - 1) : 0;
    const int rowsWithAdd = count + (count < PSU_SEQ_MAX_STEPS ? 1 : 0);
    const uint8_t maxFirst = rowsWithAdd > ROWS ? rowsWithAdd - ROWS : 0;
    if (first > maxFirst) first = maxFirst;
    char b[64];

    // ---- table
    const int running = (seq->run == PSU_SEQ_RUN || seq->run == PSU_SEQ_PAUSE) ? seq->step_index : -1;
    const int markRow = (locked ? running : sel) - first;
    if (count && markRow >= 0 && markRow < ROWS) {
        rowMark.setVisible(true);
        rowMark.moveTo(18, ROW_Y + markRow * ROW_H);
    } else if (rowMark.isVisible()) { rowMark.setVisible(false); rowMark.invalidate(); }
    for (int r = 0; r < ROWS; ++r) {
        const unsigned index = first + r;
        const bool exists = index < count;
        const PsuSeqStep* s = exists ? &seq->steps[index] : 0;
        const bool marked = r == markRow;
        for (int c = 0; c < COLS; ++c) {
            kit::Btn& btn = cell[r][c];
            btn.setBase(marked ? ui::RAISED : ui::SURFACE);
            btn.setEnabled(exists && (!locked || c == 0));
            btn.setTouchable(exists && !locked);
            if (!exists) { btn.setLabel(""); continue; }
            const bool editingThis = !cycleEditing && edit.field == columnField[c] && c && index == sel;
            if (c == 0) {
                snprintf(b, sizeof(b), "%s%u", (int)index == running ? "> " : "", index + 1);
                btn.setLabel(b);
                btn.setInk(marked ? ui::ACCENT : ui::MUTED);
            } else if (editingThis) {
                psu_seq_edit_cell(b, sizeof(b), &edit, s);
                btn.setLabel(b);
                btn.setInk(edit.fault ? ui::NEGATIVE : ui::ACCENT);
            } else if (c == 3) {
                if (s->slew_mv_per_s == 0) btn.setLabel("instant");
                else { psu_seq_format_field(b, sizeof(b), PSU_SEQ_FIELD_S, s); btn.setLabel(b); }
                btn.setInk(s->slew_mv_per_s ? ui::TEXT : ui::MUTED);
            } else {
                psu_seq_format_field(b, sizeof(b), columnField[c], s);
                btn.setLabel(b);
                btn.setInk(!s->enabled ? ui::MUTED : c == 1 ? ui::VOLT : c == 2 ? ui::AMP : ui::TEXT);
            }
            btn.setSelected(editingThis);
        }
    }
    if (count > ROWS) snprintf(b, sizeof(b), "%u-%u / %u", first + 1, first + ROWS, count);
    else snprintf(b, sizeof(b), "%u / %u", count, (unsigned)PSU_SEQ_MAX_STEPS);
    listRange.set(b);
    {
        const int slot = (int)count - first;
        const bool show = !locked && count < PSU_SEQ_MAX_STEPS && slot >= 0 && slot < ROWS;
        if (action[0].isVisible() != show || action[0].getY() != ROW_Y + slot * ROW_H + 2) {
            action[0].invalidate();
            action[0].setVisible(show);
            if (show) action[0].setY(ROW_Y + slot * ROW_H + 2);
            action[0].invalidate();
        }
    }
    action[1].setEnabled(!locked && sel > 0);
    action[2].setEnabled(!locked && sel + 1 < count);
    action[3].setEnabled(!locked && count < PSU_SEQ_MAX_STEPS);
    action[4].setEnabled(!locked && count > 1);

    uint32_t totalMs, elapsedMs;
    refreshProfile(totalMs, elapsedMs);

    // ---- header line
    if (locked) snprintf(b, sizeof(b), "Sequence active \xC2\xB7 stop it to change steps");
    else if (edit.fault) snprintf(b, sizeof(b), "Value out of range \xC2\xB7 step unchanged");
    else if (seq->status[0] && (seq->run == PSU_SEQ_ABORTED || seq->run == PSU_SEQ_DONE)) snprintf(b, sizeof(b), "%s", seq->status);
    else snprintf(b, sizeof(b), "%u step%s \xC2\xB7 tap a value to edit", count, count == 1 ? "" : "s");
    header.setSub(b, locked ? ui::CAUTION : edit.fault ? ui::NEGATIVE : ui::MUTED);

    if (editing()) {
        static const char* const captions[] = { "", "STEP VOLTAGE / V", "CURRENT LIMIT / A", "HOLD AFTER RAMP / S", "RAMP SLEW / V/S" };
        static const char* const hints[] = { "", "0 - 27 V", "0 - 5 A", "0.1 - 3600 s", "0 - 100 V/s, 0 = instant" };
        if (cycleEditing) {
            editCaption.set("REPEAT COUNT");
            editValue.set(cycleEditor.text[0] ? cycleEditor.text : "0", ui::ACCENT);
            editHint.set("1 - 65535 cycles");
        } else {
            snprintf(b, sizeof(b), "STEP %u \xC2\xB7 %s", sel + 1, captions[edit.field]);
            editCaption.set(b);
            psu_seq_edit_cell(b, sizeof(b), &edit, count ? &seq->steps[sel] : 0);
            editValue.set(b, edit.fault ? ui::NEGATIVE : ui::ACCENT);
            editHint.set(hints[edit.field]);
        }
        keys[9].setEnabled(!cycleEditing);
        ui::ThemeScreen::get().sync();
        return;
    }

    // ---- run card
    const char* state = "IDLE"; int led = ui::MUTED;
    switch (seq->run) {
    case PSU_SEQ_RUN: state = "RUNNING"; led = ui::POSITIVE; break;
    case PSU_SEQ_PAUSE: state = "PAUSED"; led = ui::CAUTION; break;
    case PSU_SEQ_DONE: state = "DONE"; led = ui::ACCENT; break;
    case PSU_SEQ_ABORTED: state = "ABORTED"; led = ui::NEGATIVE; break;
    default: break;
    }
    runChip.set(state, led);
    runChip.setX(776 - runChip.getWidth());
    clock(b, sizeof(b), elapsedMs);
    runTime.set(b);
    {
        char t[20]; clock(t, sizeof(t), totalMs);
        snprintf(b, sizeof(b), "of %s per cycle", t);
        runTotal.set(b);
    }
    progress.set(totalMs ? (int)((uint64_t)elapsedMs * 1000 / totalMs) : 0, seq->run == PSU_SEQ_PAUSE ? ui::CAUTION : ui::POSITIVE);
    if (running >= 0) {
        const unsigned loops = seq->mode == PSU_SEQ_INFINITE ? 0 : seq->mode == PSU_SEQ_ONCE ? 1 : seq->loops_requested;
        if (loops) snprintf(b, sizeof(b), "Step %u of %u\nCycle %u of %u", running + 1, count, seq->loop_index + 1, loops);
        else snprintf(b, sizeof(b), "Step %u of %u\nCycle %u \xC2\xB7 continuous", running + 1, count, seq->loop_index + 1);
    } else snprintf(b, sizeof(b), "%s\nRUN switches the output on", seq->run == PSU_SEQ_DONE ? "Finished" : seq->run == PSU_SEQ_ABORTED ? "Stopped early" : "Ready");
    runInfo.set(b);

    const int m = seq->mode == PSU_SEQ_INFINITE ? 2 : seq->mode == PSU_SEQ_N ? 1 : 0;
    for (int i = 0; i < 3; ++i) { mode[i].setSelected(m == i); mode[i].setEnabled(!locked); }
    snprintf(b, sizeof(b), "%u\xC3\x97", seq->mode == PSU_SEQ_ONCE ? 1u : (unsigned)seq->loops_requested);
    cycleCount.setLabel(seq->mode == PSU_SEQ_INFINITE ? "loop" : b);
    cycleLess.setEnabled(!locked && seq->mode != PSU_SEQ_INFINITE && seq->loops_requested > 1);
    cycleMore.setEnabled(!locked && seq->mode != PSU_SEQ_INFINITE && seq->loops_requested < 65535);
    cycleCount.setEnabled(!locked && seq->mode != PSU_SEQ_INFINITE);

    if (seq->run == PSU_SEQ_RUN) { runBtn.setLabel("PAUSE"); runBtn.setStyle(kit::Btn::NORMAL); runBtn.setEnabled(true); }
    else if (seq->run == PSU_SEQ_PAUSE) { runBtn.setLabel("RESUME"); runBtn.setStyle(kit::Btn::POSITIVE_FILL); runBtn.setEnabled(true); }
    else { runBtn.setLabel("RUN"); runBtn.setStyle(kit::Btn::POSITIVE_FILL); runBtn.setEnabled(count > 0 && !psu_charger()->running); }
    stopBtn.setEnabled(locked);
    ui::ThemeScreen::get().sync();
}

void ScreenSequencerView::select(uint8_t index)
{
    PsuSequencer* seq = psu_sequencer();
    if (index >= seq->count) return;
    seq->selected = index;
    if (index < first) first = index;
    if (index >= first + ROWS) first = index - ROWS + 1;
}

void ScreenSequencerView::move(int direction)
{
    PsuSequencer* seq = psu_sequencer();
    if (psu_seq_edit_locked(seq)) return;
    const int from = seq->selected, to = from + direction;
    if (to < 0 || to >= seq->count) return;
    const PsuSeqStep t = seq->steps[from];
    seq->steps[from] = seq->steps[to];
    seq->steps[to] = t;
    select((uint8_t)to);
}

void ScreenSequencerView::duplicate()
{
    PsuSequencer* seq = psu_sequencer();
    if (psu_seq_edit_locked(seq) || seq->count >= PSU_SEQ_MAX_STEPS || !seq->count) return;
    const uint8_t at = seq->selected + 1;
    for (int i = seq->count; i > at; --i) seq->steps[i] = seq->steps[i - 1];
    seq->steps[at] = seq->steps[at - 1];
    ++seq->count;
    select(at);
}

void ScreenSequencerView::run()
{
    PsuSequencer* seq = psu_sequencer();
    if (seq->run == PSU_SEQ_RUN) { psu_seq_pause(seq, psu_app_now()); return; }
    if (seq->run == PSU_SEQ_PAUSE) { psu_seq_resume(seq, psu_app_now()); return; }
    if (editing() || psu_charger()->running) return;
    // RUN explicitly starts the output; KEEP is retained for later steps.
    for (unsigned i = 0; i < seq->count; i++)
        if (seq->steps[i].enabled) { if (seq->steps[i].output_action == PSU_STEP_KEEP) seq->steps[i].output_action = PSU_STEP_ON; break; }
    psu_seq_start(seq, psu_app_now());
}

void ScreenSequencerView::keyPressed(int k)
{
    PsuSequencer* seq = psu_sequencer();
    if (cycleEditing) {
        if (k <= 10 && k != 9) psu_editor_key(&cycleEditor, keyLabels[k][0]);
        else if (k == 11) psu_editor_backspace(&cycleEditor);
        else if (k == 12) psu_editor_clear(&cycleEditor);
        else if (k == 13) {
            uint32_t value;
            if (psu_editor_parse_milli(&cycleEditor, &value)) {
                value /= 1000;
                if (value < 1) value = 1;
                if (value > 65535) value = 65535;
                seq->loops_requested = (uint16_t)value;
                seq->mode = value == 1 ? PSU_SEQ_ONCE : PSU_SEQ_N;
                cycleEditing = false;
                showEditor(false);
            }
        }
        return;
    }
    if (k <= 10) psu_seq_edit_key(&edit, seq, keyLabels[k][0]);
    else if (k == 11) psu_seq_edit_backspace(&edit, seq);
    else if (k == 12) psu_seq_edit_clear(&edit, seq);
    else if (psu_seq_edit_apply(&edit, seq)) { edit.field = PSU_SEQ_FIELD_NONE; showEditor(false); }
}

void ScreenSequencerView::onClick(const kit::Btn& b)
{
    PsuSequencer* seq = psu_sequencer();
    const int t = b.tag;
    const bool locked = psu_seq_edit_locked(seq) != 0;
    if (t >= 100 && t < 200) {
        if (listDragging || locked) return;
        const int r = (t - 100) / 8, c = (t - 100) % 8;
        const unsigned index = first + r;
        if (index >= seq->count) return;
        const bool same = index == seq->selected && edit.field == columnField[c];
        select((uint8_t)index);
        cycleEditing = false;
        if (c == 0 || same) { edit.field = PSU_SEQ_FIELD_NONE; edit.fault = 0; showEditor(false); }
        else { psu_seq_edit_select(&edit, seq, columnField[c]); showEditor(true); }
    } else if (t >= 200 && t < 300) {
        if (locked) return;
        edit.field = PSU_SEQ_FIELD_NONE; cycleEditing = false; showEditor(false);
        switch (t - 200) {
        case 0: psu_seq_add(seq); select(seq->selected); break;
        case 1: move(-1); break;
        case 2: move(1); break;
        case 3: duplicate(); break;
        case 4: psu_seq_remove_selected(seq); select(seq->selected); break;
        }
    } else if (t >= 300 && t < 303) {
        if (locked) return;
        if (t == 300) { seq->mode = PSU_SEQ_ONCE; }
        else if (t == 301) { if (seq->loops_requested < 2) seq->loops_requested = 2; seq->mode = PSU_SEQ_N; }
        else seq->mode = PSU_SEQ_INFINITE;
    } else if (t == 310 || t == 312) {
        if (locked || seq->mode == PSU_SEQ_INFINITE) return;
        if (t == 310 && seq->loops_requested > 1) --seq->loops_requested;
        if (t == 312 && seq->loops_requested < 65535) ++seq->loops_requested;
        seq->mode = seq->loops_requested <= 1 ? PSU_SEQ_ONCE : PSU_SEQ_N;
    } else if (t == 311) {
        if (locked) return;
        edit.field = PSU_SEQ_FIELD_NONE;
        cycleEditing = true;
        psu_editor_load_milli(&cycleEditor, (seq->mode == PSU_SEQ_ONCE ? 1u : seq->loops_requested) * 1000U, 0);
        showEditor(true);
    } else if (t == 400) {
        run();
    } else if (t == 401) {
        psu_seq_stop(seq, psu_app_now(), 0);
    } else if (t >= 500) {
        keyPressed(t - 500);
    }
    refresh();
}

void ScreenSequencerView::scroll(int delta)
{
    const int count = psu_sequencer()->count + (psu_sequencer()->count < PSU_SEQ_MAX_STEPS ? 1 : 0);
    int f = first + delta;
    if (f > count - ROWS) f = count - ROWS;
    if (f < 0) f = 0;
    if (f != first) { first = (uint8_t)f; refresh(); }
}

void ScreenSequencerView::handleClickEvent(const touchgfx::ClickEvent& e)
{
    if (e.getType() == touchgfx::ClickEvent::PRESSED) { listDragging = false; dragPixels = 0; }
    if (e.getType() == touchgfx::ClickEvent::RELEASED && listDragging) {
        ScreenSequencerViewBase::handleClickEvent(touchgfx::ClickEvent(touchgfx::ClickEvent::CANCEL, e.getX(), e.getY()));
        return;
    }
    ScreenSequencerViewBase::handleClickEvent(e);
}

void ScreenSequencerView::handleDragEvent(const touchgfx::DragEvent& e)
{
    if (e.getOldX() < 568 && e.getOldY() >= ROW_Y && e.getOldY() < ROW_Y + ROWS * ROW_H) {
        dragPixels += e.getDeltaY();
        if (dragPixels <= -12 || dragPixels >= 12) listDragging = true;
        while (dragPixels <= -ROW_H) { scroll(1); dragPixels += ROW_H; }
        while (dragPixels >= ROW_H) { scroll(-1); dragPixels -= ROW_H; }
    } else ScreenSequencerViewBase::handleDragEvent(e);
}

void ScreenSequencerView::allOff() { psu_app_power_shutdown(); }
