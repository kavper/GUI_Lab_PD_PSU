#include <gui/screendiagnostics_screen/ScreenDiagnosticsView.hpp>
#include <gui/common/ErrorCatalog.hpp>
#include <gui/common/KitPage.hpp>
#include <gui/common/UiTheme.hpp>

namespace {
void formatValue(char* out, size_t n, const char* key, int64_t val)
{
    const char* suffix = strrchr(key, '_');
    const char* unit = 0;
    if (suffix && !strcmp(suffix, "_mv")) unit = "V";
    if (suffix && !strcmp(suffix, "_ma")) unit = "A";
    if (suffix && !strcmp(suffix, "_mw")) unit = "W";
    if (unit) { uint64_t a = val < 0 ? -val : val; snprintf(out, n, "%s%llu.%03llu %s", val < 0 ? "-" : "", (unsigned long long)(a / 1000), (unsigned long long)(a % 1000), unit); }
    else if (suffix && !strcmp(suffix, "_ms")) snprintf(out, n, "%lld ms", (long long)val);
    else if (suffix && !strcmp(suffix, "_x10")) snprintf(out, n, "%lld.%lld %%", (long long)(val / 10), (long long)(val % 10));
    else if (!strcmp(key, "mode")) snprintf(out, n, "%s", val == 2 ? "CC" : val == 1 ? "CV" : "IDLE");
    else if (!strcmp(key, "pd_role")) snprintf(out, n, "%s", val == 1 ? "SINK" : val == 2 ? "SOURCE" : "NONE");
    else if (strstr(key, "fault") || !strcmp(key, "sa") || !strcmp(key, "sb") || !strcmp(key, "sc") || !strcmp(key, "alarm")) snprintf(out, n, "0x%lX", (unsigned long)val);
    else snprintf(out, n, "%lld", (long long)val);
}
const unsigned ROWS = 13;
}

ScreenDiagnosticsView::ScreenDiagnosticsView() : kind(0), offset(0), raw(false), divider(0), drag(0), clicked(this, &ScreenDiagnosticsView::onClick) {}

void ScreenDiagnosticsView::setupScreen()
{
    ScreenDiagnosticsViewBase::setupScreen();
    layout();
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this); theme.box(LabBackground, ui::BACKGROUND); theme.apply();
    refresh();
}

void ScreenDiagnosticsView::layout()
{
    KIT_PAGE_BEGIN(FrontendApplication::DIAGNOSTICS);
    tabs.setup(12, 58, 776, 46, ui::RAISED, ui::BORDER, ui::BACKGROUND, 8);
    add(tabs);
    static const char* const names[] = { "METER", "SENSE", "BMS", "PD", "PARSED", "RAW UART" };
    for (int i = 0; i < 6; ++i) {
        tab[i].setup(16 + i * 128, 62, 126, 38, names[i], kit::Btn::SEGMENT, kit::BODYM);
        tab[i].setBase(ui::RAISED); tab[i].tag = i; tab[i].setAction(clicked);
        add(tab[i]);
    }
    fieldsLeft.setup(12, 114, 262, 354, "");
    fieldsRight.setup(282, 114, 258, 354, "");
    linkCard.setup(550, 114, 238, 142, "G4 LINK");
    eventCard.setup(550, 266, 238, 144, "LAST EVENT");
    add(fieldsLeft); add(fieldsRight); add(linkCard); add(eventCard);
    left.setup(24, 124, 240, ROWS * 25, 25);
    right.setup(294, 124, 236, ROWS * 25, 25);
    add(left); add(right);
    static const char* const counters[] = { "RX frames", "ACK", "Bad", "Errors" };
    for (int i = 0; i < 4; ++i) {
        counter[i].setup(560 + (i % 2) * 110, 136 + (i / 2) * 58, 104, 52, counters[i], kit::Btn::FIELD, kit::SUB);
        counter[i].setBase(ui::SURFACE); counter[i].setTouchable(false);
        add(counter[i]);
    }
    event.setup(562, 290, 216, 112, 16);
    event.setMonoPlain(true);
    add(event);
    clearBtn.setup(550, 418, 238, 50, "CLEAR FAULT", kit::Btn::NORMAL, kit::BODYM);
    clearBtn.setBase(ui::BACKGROUND); clearBtn.tag = 10; clearBtn.setAction(clicked);
    add(clearBtn);
    kit::addNoSwipe(touchgfx::Rect(12, 114, 528, 354));
    KIT_PAGE_HEADER(FrontendApplication::DIAGNOSTICS);
}

void ScreenDiagnosticsView::onClick(const kit::Btn& b)
{
    static const unsigned kinds[] = { G4_RECORD_T, G4_RECORD_AUX, G4_RECORD_TB, G4_RECORD_TC };
    if (b.tag < 4) { kind = kinds[b.tag]; offset = 0; }
    else if (b.tag == 4) { raw = false; offset = 0; }
    else if (b.tag == 5) { raw = true; offset = 0; }
    else if (b.tag == 10) { clearFault(); return; }
    refresh();
}

void ScreenDiagnosticsView::handleTickEvent()
{
    if (kit::app()->isScreenTransitionActive()) return;
    if (++divider >= 12) { divider = 0; refresh(); }
}

void ScreenDiagnosticsView::refresh()
{
    static const unsigned kinds[] = { G4_RECORD_T, G4_RECORD_AUX, G4_RECORD_TB, G4_RECORD_TC };
    for (int i = 0; i < 4; ++i) tab[i].setSelected(kind == kinds[i]);
    tab[4].setSelected(!raw); tab[5].setSelected(raw);

    TelemetryRecord rec(kind);
    G4Port* host = psu_g4();
    static char text[600], text2[600];
    char b[96];
    text[0] = text2[0] = 0;
    unsigned count = 0;
    while (g4_record_key(kind, count)) ++count;
    if (raw) {
        static char source[G4_RX_LINE_MAX];
        g4_raw_snapshot(host, kind, source, sizeof(source));
        const unsigned columns = 24, lines = (strlen(source) + columns - 1) / columns;
        const unsigned maxOffset = lines > ROWS * 2 ? lines - ROWS * 2 : 0;
        if (offset > maxOffset) offset = maxOffset;
        for (unsigned l = 0; l < ROWS * 2 && (offset + l) * columns < strlen(source); ++l) {
            char* dst = l < ROWS ? text : text2;
            size_t used = strlen(dst);
            snprintf(dst + used, 600 - used, "%.*s\n", (int)columns, source + (offset + l) * columns);
        }
        if (!source[0]) strcpy(text, "Waiting for a complete\nUART frame");
        left.setMonoPlain(true); right.setMonoPlain(true);
    } else {
        const unsigned maxOffset = count > ROWS * 2 ? count - ROWS * 2 : 0;
        if (offset > maxOffset) offset = maxOffset;
        for (unsigned i = 0; i < ROWS * 2; ++i) {
            const char* key = g4_record_key(kind, offset + i);
            if (!key) break;
            int64_t val;
            char v[40];
            if (rec.get(key, val)) formatValue(v, sizeof(v), key, val); else strcpy(v, "--");
            TelemetryRecord::append(i < ROWS ? text : text2, 600, key, v);
        }
        left.setMonoPlain(false); right.setMonoPlain(false);
    }
    left.set(text, rec.fresh);
    right.set(text2, rec.fresh);

    snprintf(b, sizeof(b), "%lu", (unsigned long)host->rx_lines); counter[0].setDetail(b, kit::M16);
    snprintf(b, sizeof(b), "%lu", (unsigned long)host->ack_count); counter[1].setDetail(b, kit::M16);
    snprintf(b, sizeof(b), "%lu", (unsigned long)host->parse_error_count); counter[2].setDetail(b, kit::M16);
    snprintf(b, sizeof(b), "%lu", (unsigned long)host->err_count); counter[3].setDetail(b, kit::M16);
    counter[2].setInk(host->parse_error_count ? ui::CAUTION : ui::POSITIVE);
    counter[3].setInk(host->err_count ? ui::NEGATIVE : ui::POSITIVE);

    PsuSnapshot live; psu_snapshot(&live);
    const char* phase = live.output_phase == G4_OUTPUT_STARTING ? "START" : live.output_phase == G4_OUTPUT_RUNNING ? "RUN" : live.output_phase == G4_OUTPUT_STOPPING ? "STOP" : "OFF";
    static char ev[600];
    const char* last = live.fault_latched ? live.fault_context : (live.command_error[0] ? live.command_error : (live.last_on_reject[0] ? live.last_on_reject : "none"));
    snprintf(ev, sizeof(ev), "%.32s\nH7 %s%s: %.20s\nG4 fault 0x%lX ctrl %u latch %u\nG0 fault 0x%lX kill %u\n",
             last, phase, live.fault_latched ? " LATCH" : "", live.on_block_reason,
             (unsigned long)host->telemetry.fault, host->telemetry.ctrl, host->telemetry.fault_latch,
             (unsigned long)host->telemetry.g0_fault, host->telemetry.kill);
    size_t used = strlen(ev);
    if (host->nack_valid) { const err::Entry* e = err::find(err::E_NACK, host->nack_reason);
        snprintf(ev + used, sizeof(ev) - used, "NACK 0x%02X: %s\n", host->nack_type, e ? e->text : "unknown reason"); }
    else snprintf(ev + used, sizeof(ev) - used, "Last NACK: none\n");
    event.set(ev, true);
    eventCard.setBorder(live.fault_latched ? ui::NEGATIVE : ui::BORDER);
    clearBtn.setEnabled(live.fault_latched && host->telemetry.valid && psu_app_now() - host->telemetry.ms <= G4_METER_FRESH_MS);

    snprintf(b, sizeof(b), "%s \xC2\xB7 %s \xC2\xB7 %u fields \xC2\xB7 drag to scroll", kind == 0 ? "METER" : kind == 1 ? "BMS" : kind == 2 ? "PD" : "SENSE", rec.fresh ? "live" : "stale", count);
    header.setSub(b, rec.fresh ? ui::MUTED : ui::CAUTION);
}

void ScreenDiagnosticsView::allOff() { psu_app_power_shutdown(); }
void ScreenDiagnosticsView::clearFault() { psu_app_clear_fault(); refresh(); }
void ScreenDiagnosticsView::handleDragEvent(const touchgfx::DragEvent& e)
{
    if (e.getOldY() >= 114 && e.getOldX() < 540) {
        drag += e.getDeltaY();
        while (drag <= -25) { ++offset; drag += 25; refresh(); }
        while (drag >= 25) { if (offset) --offset; drag -= 25; refresh(); }
    } else ScreenDiagnosticsViewBase::handleDragEvent(e);
}
