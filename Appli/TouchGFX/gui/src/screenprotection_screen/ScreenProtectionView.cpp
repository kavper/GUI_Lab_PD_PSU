#include <gui/screenprotection_screen/ScreenProtectionView.hpp>
#include <gui/common/ErrorCatalog.hpp>
#include <gui/common/KitPage.hpp>
#include <gui/common/UiTheme.hpp>

ScreenProtectionView::ScreenProtectionView() : divider(0), noticeTicks(0), clicked(this, &ScreenProtectionView::onClick) { notice[0] = 0; }

void ScreenProtectionView::setupScreen()
{
    ScreenProtectionViewBase::setupScreen();
    layout();
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this); theme.box(LabBackground, ui::BACKGROUND); theme.apply();
    refresh();
}
void ScreenProtectionView::tearDownScreen() { ScreenProtectionViewBase::tearDownScreen(); }

void ScreenProtectionView::layout()
{
    KIT_PAGE_BEGIN(FrontendApplication::PROTECTION);
    banner.setup(12, 58, 776, 58, ui::SURFACE, ui::POSITIVE, ui::BACKGROUND, 11);
    add(banner);
    bannerIcon.setup(20, 64, 44, 46, "", kit::Btn::GHOST);
    bannerIcon.setBase(ui::SURFACE); bannerIcon.setIcon(kit::ICON_SHIELD); bannerIcon.setTouchable(false); bannerIcon.setIconSize(24);
    add(bannerIcon);
    bannerTitle.setup(kit::TITLE, kit::LEFT, 72, 66, 540, 24, ui::POSITIVE);
    bannerSub.setup(kit::SUB, kit::LEFT, 72, 90, 540, 16, ui::MUTED);
    add(bannerTitle); add(bannerSub);
    clearBtn.setup(640, 66, 138, 42, "CLEAR FAULT", kit::Btn::NORMAL, kit::BODYM);
    clearBtn.setBase(ui::SURFACE); clearBtn.tag = 1; clearBtn.setAction(clicked);
    add(clearBtn);
    static const char* const captions[] = { "G4 SUPERVISOR", "G0 LDO", "GATE DRIVER", "LDO PERMIT" };
    for (int i = 0; i < 4; ++i) {
        status[i].setup(12 + i * 196, 126, 186, 76, captions[i], kit::M20, kit::M12);
        add(status[i]);
    }
    faultsCard.setup(12, 212, 488, 256, "ACTIVE FAULTS", "G4 fault bits");
    linkCard.setup(510, 212, 278, 256, "LINK HEALTH");
    add(faultsCard); add(linkCard);
    faultRows.setup(25, 238, 462, 220, 24);
    linkRows.setup(523, 238, 252, 150, 22);
    add(faultRows); add(linkRows);
    refreshBtn.setup(522, 412, 254, 44, "REFRESH", kit::Btn::NORMAL, kit::BODYM);
    refreshBtn.setBase(ui::SURFACE); refreshBtn.tag = 2; refreshBtn.setAction(clicked);
    add(refreshBtn);
    KIT_PAGE_HEADER(FrontendApplication::PROTECTION);
}

void ScreenProtectionView::handleTickEvent()
{
    if (kit::app()->isScreenTransitionActive()) return;
    if (noticeTicks) --noticeTicks;
    if (++divider >= 4) { divider = 0; refresh(); }
}
void ScreenProtectionView::notify(const char* text) { snprintf(notice, sizeof(notice), "%s", text); noticeTicks = 180; refresh(); }
void ScreenProtectionView::allOff() { psu_app_power_shutdown(); notify("POWER OFF requested - BMS shutdown / wake with TS2"); }
void ScreenProtectionView::clearFault() { notify(psu_app_clear_fault() ? "CLR requested; confirm that reported faults clear" : "No PSU fault to clear"); }
void ScreenProtectionView::refreshTelemetry() { notify(g4_simple(psu_g4(), "STATUS", 0) ? "PING sent - telemetry streams automatically" : "Command queue full"); }

void ScreenProtectionView::refresh()
{
    TelemetryRecord r(G4_RECORD_T);
    PsuSnapshot live; psu_snapshot(&live);
    char b[96], rows[400];
    int64_t g4 = 0, g0 = 0, flt = 0, permit = 0, ps = 0;
    const bool haveG4 = r.get("fault", g4), haveG0 = r.get("g0_fault", g0), haveFlt = r.get("flt", flt), havePermit = r.get("permit", permit);
    const bool fault = (haveG4 && g4) || (haveG0 && g0) || (haveFlt && flt) || live.fault_latched;

    banner.setBorder(!r.fresh ? ui::BORDER : fault ? ui::NEGATIVE : ui::POSITIVE);
    bannerIcon.setInk(!r.fresh ? ui::MUTED : fault ? ui::NEGATIVE : ui::POSITIVE);
    bannerTitle.set(!r.fresh ? "Waiting for protection telemetry" : fault ? "Protection fault active" : "All protections clear",
                    !r.fresh ? ui::MUTED : fault ? ui::NEGATIVE : ui::POSITIVE);
    TelemetryRecord aux(G4_RECORD_AUX);
    char issue[64];
    const int severity = err::outputIssue(r, aux, issue, sizeof(issue));
    char latch[64];
    if (live.fault_latched) snprintf(b, sizeof(b), "Output blocked \xC2\xB7 %s", err::latchText(live.fault, latch, sizeof(latch)));
    else if (fault && severity >= 0) snprintf(b, sizeof(b), "%s \xC2\xB7 clear it, then CLEAR", issue);
    else if (fault) snprintf(b, sizeof(b), "Output blocked until the fault clears and CLEAR is confirmed");
    else snprintf(b, sizeof(b), "Output %s", havePermit && permit ? "permitted" : "not permitted by the LDO");
    bannerSub.set(b);
    clearBtn.setEnabled(r.fresh && fault);

    status[0].set(!haveG4 ? "--" : g4 ? "FAULT" : "CLEAR", !haveG4 ? ui::MUTED : g4 ? ui::NEGATIVE : ui::POSITIVE);
    if (g4) err::symbols(err::E_PSU, (uint32_t)g4, b, sizeof(b)); else strcpy(b, "no faults"); status[0].setDetail(haveG4 ? b : "--");
    status[1].set(!haveG0 ? "--" : g0 ? "FAULT" : "CLEAR", !haveG0 ? ui::MUTED : g0 ? ui::NEGATIVE : ui::POSITIVE);
    if (g0) err::symbols(err::E_G0, (uint32_t)g0, b, sizeof(b)); else strcpy(b, "no faults"); status[1].setDetail(haveG0 ? b : "--");
    status[2].set(!haveFlt ? "--" : flt ? "FAULT" : "CLEAR", !haveFlt ? ui::MUTED : flt ? ui::NEGATIVE : ui::POSITIVE);
    if (r.get("ps_err", ps)) snprintf(b, sizeof(b), ps ? "power stage err %ld" : "power stage ok", (long)ps); else strcpy(b, "--");
    status[2].setDetail(b);
    status[3].set(!havePermit ? "--" : permit ? "ALLOWED" : "BLOCKED", !havePermit ? ui::MUTED : permit ? ui::POSITIVE : ui::CAUTION);
    status[3].setDetail(live.output_confirmed ? "output on" : "output off");
    for (int i = 0; i < 3; ++i) status[i].setBorder(i == 0 ? (haveG4 && g4 ? ui::NEGATIVE : ui::BORDER) : i == 1 ? (haveG0 && g0 ? ui::NEGATIVE : ui::BORDER) : (haveFlt && flt ? ui::NEGATIVE : ui::BORDER));

    rows[0] = 0;
    if (!haveG4) strcpy(rows, "Waiting for fault telemetry.\n");
    else {
        // Plain-language list of everything that is set (G4, G0, BMS cause).
        const err::Entry* list[8]; uint32_t unknown = 0;
        unsigned n = err::active(err::E_PSU, (uint32_t)g4, list, 8, &unknown);
        for (unsigned i = 0; i < n; ++i) TelemetryRecord::append(rows, sizeof(rows), list[i]->text, "");
        if (haveG0) { const unsigned m = err::active(err::E_G0, (uint32_t)g0, list, 8); for (unsigned i = 0; i < m; ++i) TelemetryRecord::append(rows, sizeof(rows), list[i]->text, ""); n += m; }
        if (g4 & 0x20) { TelemetryRecord pack(G4_RECORD_TB); char why[64]; if (err::bmsIssue(pack, why, sizeof(why)) >= 0) { TelemetryRecord::append(rows, sizeof(rows), why, ""); ++n; } }
        if (unknown) { snprintf(b, sizeof(b), "0x%lX", (unsigned long)unknown); TelemetryRecord::append(rows, sizeof(rows), "Unknown G4 bits", b); }
        if (!n && !unknown) strcpy(rows, "No faults reported.\n");
        int64_t stop = 0;
        if (aux.get("stop_reason", stop) && stop) { const err::Entry* e = err::find(err::E_STOP, (uint32_t)stop); TelemetryRecord::append(rows, sizeof(rows), "Last stop", e ? e->text : "unknown"); }
    }
    TelemetryRecord::append(rows, sizeof(rows), "Output latch", live.fault_latched ? "SET" : "CLEAR");
    if (live.fault_context[0]) { size_t n = strlen(rows); snprintf(rows + n, sizeof(rows) - n, "%.60s\n", live.fault_context); }
    faultRows.set(rows, r.fresh);

    rows[0] = 0;
    r.row(rows, sizeof(rows), "G0 UART errors", "g0_err", "", 1, 0);
    r.hex(rows, sizeof(rows), "G0 UART flags", "g0_uart");
    r.row(rows, sizeof(rows), "Power-stage error", "ps_err", "", 1, 0);
    snprintf(b, sizeof(b), "%lu", (unsigned long)live.g0_crc_errors);
    TelemetryRecord::append(rows, sizeof(rows), "G0 CRC errors", b);
    TelemetryRecord::append(rows, sizeof(rows), "G4 link", live.g4_link ? "ONLINE" : "OFFLINE");
    linkRows.set(rows, r.fresh);

    if (noticeTicks) header.setSub(notice, ui::TEXT);
    else if (!r.record.valid) header.setSub("Waiting for METER telemetry", ui::CAUTION);
    else { snprintf(b, sizeof(b), "%s %lu ms ago", r.fresh ? "updated" : "STALE", (unsigned long)(psu_app_now() - r.record.ms)); header.setSub(b, r.fresh ? ui::MUTED : ui::CAUTION); }
}
