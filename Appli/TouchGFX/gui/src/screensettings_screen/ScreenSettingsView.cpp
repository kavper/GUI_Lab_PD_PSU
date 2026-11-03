#include <gui/screensettings_screen/ScreenSettingsView.hpp>
#include <gui/common/UsbSession.hpp>
#include <gui/common/ErrorCatalog.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/UiTheme.hpp>
#include <stdio.h>
extern "C" {
#include "psu_app.h"
}

namespace {
struct TileDef { FrontendApplication::Page page; const char* title; kit::Icon icon; };
const TileDef tileDefs[] = {
    { FrontendApplication::MAIN, "Output", kit::ICON_OUTPUT },
    { FrontendApplication::BATTERY, "Battery", kit::ICON_BATTERY },
    { FrontendApplication::BMS, "Cells / BMS", kit::ICON_CELLS },
    { FrontendApplication::USB_PD, "USB-C", kit::ICON_USB },
    { FrontendApplication::OUTPUT, "Output path", kit::ICON_PATH },
    { FrontendApplication::SENSE, "Sense & temps", kit::ICON_TEMP },
    { FrontendApplication::PROTECTION, "Protection", kit::ICON_SHIELD },
    { FrontendApplication::PRESETS, "Presets", kit::ICON_LIST },
    { FrontendApplication::SEQUENCER, "Sequencer", kit::ICON_SEQ },
    { FrontendApplication::CHARGER, "Cell charger", kit::ICON_BOLT },
    { FrontendApplication::DIAGNOSTICS, "Diagnostics", kit::ICON_DIAG },
    { FrontendApplication::NETWORK, "Network", kit::ICON_NET },
    { FrontendApplication::APPEARANCE, "Appearance", kit::ICON_THEME },
    { FrontendApplication::SERVICE, "Service", kit::ICON_SERVICE },
};
FrontendApplication* app() { return static_cast<FrontendApplication*>(touchgfx::Application::getInstance()); }
void mv(char* b, size_t n, int64_t v, unsigned dec = 2)
{
    const int64_t a = v < 0 ? -v : v;
    if (dec == 2) snprintf(b, n, "%s%ld.%02ld", v < 0 ? "-" : "", (long)(a / 1000), (long)(a % 1000 / 10));
    else snprintf(b, n, "%s%ld.%03ld", v < 0 ? "-" : "", (long)(a / 1000), (long)(a % 1000));
}
}

ScreenSettingsView::ScreenSettingsView() : divider(0), armTicks(0),
    tileCallback(this, &ScreenSettingsView::onTile), powerCallback(this, &ScreenSettingsView::onPower),
    closeCallback(this, &ScreenSettingsView::onClose) {}

void ScreenSettingsView::setupScreen()
{
    ScreenSettingsViewBase::setupScreen();
    app()->setSwipePage(FrontendApplication::MENU);
    kit::clearNoSwipe();
    kit::hideAll(getRootContainer());
    dim.setup(0, 0, 800, 480, ui::BACKGROUND);
    add(dim);
    sheet.setup(0, -14, 800, 466, ui::SURFACE, ui::BORDER, ui::BACKGROUND, 12);
    add(sheet);
    chipOut.setXY(16, 14); chipV.setXY(0, 14); chipA.setXY(0, 14); chipW.setXY(0, 14);
    add(chipOut); add(chipV); add(chipA); add(chipW);
    power.setup(640, 8, 148, 38, "POWER OFF", kit::Btn::DANGER, kit::BODYM);
    power.setBase(ui::SURFACE);
    power.setIcon(kit::ICON_POWER);
    power.setAction(powerCallback);
    add(power);
    monitorCaption.setup(kit::CAP, kit::LEFT, 16, 62, 300, 14, ui::MUTED);
    monitorCaption.set("MONITOR");
    toolsCaption.setup(kit::CAP, kit::LEFT, 16, 248, 300, 14, ui::MUTED);
    toolsCaption.set("TOOLS & SYSTEM");
    add(monitorCaption); add(toolsCaption);
    for (int i = 0; i < TILE_COUNT; ++i) {
        int x, y, w = 186, h = 72;
        if (i < 8) { x = 16 + (i % 4) * 194; y = 82 + (i / 4) * 80; }
        else if (i < 12) { x = 16 + (i - 8) * 194; y = 268; }
        else { x = 16 + (i - 12) * 388; y = 368; w = 380; h = 46; }
        tiles[i].setup(x, y, w, h, tileDefs[i].title, kit::Btn::FIELD, kit::BODYM);
        tiles[i].setBase(ui::SURFACE);
        tiles[i].setIcon(tileDefs[i].icon);
        tiles[i].setTitleInk(ui::TEXT);
        tiles[i].tag = i;
        tiles[i].setAction(tileCallback);
        if (i < 12) tiles[i].setDetail("--", kit::M12);
        else tiles[i].setAlign(kit::LEFT);
        add(tiles[i]);
    }
    handle.setup(376, 440, 48, 5, ui::MUTED);
    add(handle);
    closeStrip.setup(0, 452, 800, 28, "Swipe up or tap outside to close", kit::Btn::GHOST, kit::SUB);
    closeStrip.setAction(closeCallback);
    add(closeStrip);
    refresh();
}

void ScreenSettingsView::tearDownScreen()
{
    ScreenSettingsViewBase::tearDownScreen();
}

void ScreenSettingsView::handleTickEvent()
{
    if (app()->isScreenTransitionActive()) return;
    if (armTicks && --armTicks == 0) { power.setStyle(kit::Btn::DANGER); power.setLabel("POWER OFF"); }
    if (++divider >= 15) { divider = 0; refresh(); }
}

void ScreenSettingsView::onTile(const kit::Btn& b)
{
    const FrontendApplication::Page target = tileDefs[b.tag].page;
    app()->go(target, FrontendApplication::UP);
}

void ScreenSettingsView::onClose(const kit::Btn&) { app()->closeMenu(); }

void ScreenSettingsView::onPower(const kit::Btn&)
{
    if (!armTicks) { armTicks = 180; power.setStyle(kit::Btn::DANGER_FILL); power.setLabel("TAP TO CONFIRM"); return; }
    armTicks = 0;
    allOff();
}

void ScreenSettingsView::allOff()
{
    const bool ok = psu_app_power_shutdown();
    power.setStyle(kit::Btn::DANGER);
    power.setLabel(ok ? "OFF REQUESTED" : "OFF FAILED");
}

void ScreenSettingsView::refresh()
{
    PsuSnapshot s;
    psu_snapshot(&s);
    char a[32], b[32], t[48];
    const bool live = s.g0_connected && !s.g0_stale;
    chipOut.set(s.output_confirmed ? (s.mode_cc ? "OUT ON · CC" : "OUT ON · CV") : "OUT OFF", s.output_confirmed ? ui::POSITIVE : ui::MUTED);
    mv(a, sizeof(a), s.vout_mv, 3); snprintf(t, sizeof(t), "%s V", live ? a : "--"); chipV.set(t, -1, ui::VOLT);
    const int32_t ua = s.display_current_ua > 0 ? s.display_current_ua : 0;
    mv(a, sizeof(a), ua / 1000, 3); snprintf(t, sizeof(t), "%s A", live && s.current_valid ? a : "--"); chipA.set(t, -1, ui::AMP);
    const uint64_t mw = (uint64_t)s.vout_mv * (uint64_t)ua / 1000000ULL;
    mv(a, sizeof(a), (int64_t)mw); snprintf(t, sizeof(t), "%s W", live && s.current_valid ? a : "--"); chipW.set(t, -1, ui::WATT);
    chipV.setX(chipOut.getX() + chipOut.getWidth() + 6);
    chipA.setX(chipV.getX() + chipV.getWidth() + 6);
    chipW.setX(chipA.getX() + chipA.getWidth() + 6);
    touchgfx::Rect chipRow(0, 8, 630, 34);
    getRootContainer().invalidateRect(chipRow);

    // Live detail line for every tile.
    mv(a, sizeof(a), s.requested_mv); mv(b, sizeof(b), s.requested_ma);
    snprintf(t, sizeof(t), "%s V · %s A", a, b); tiles[0].setDetail(t, kit::M12);
    TelemetryRecord pack(G4_RECORD_TB), gauge(G4_RECORD_BATT), usb(G4_RECORD_TC), rail(G4_RECORD_T), aux(G4_RECORD_AUX);
    int64_t soc = 0, pmv = 0, pma = 0, lo = 0, hi = 0;
    const bool haveSoc = gauge.get("soc_permille", soc) || pack.get("soc_permille", soc);
    if (pack.get("pack_mv", pmv) && pack.get("i_pack_ma", pma)) {
        const int64_t w = pmv * pma / 1000;
        mv(a, sizeof(a), w, 2);
        if (haveSoc) snprintf(t, sizeof(t), "%ld%% · %s%s W", (long)((soc + 5) / 10), w > 0 ? "+" : "", a);
        else snprintf(t, sizeof(t), "%s%s W", w > 0 ? "+" : "", a);
    } else snprintf(t, sizeof(t), haveSoc ? "%ld%%" : "No BMS data", (long)((soc + 5) / 10));
    tiles[1].setDetail(t, kit::M12);
    if (pack.get("min_mv", lo) && pack.get("max_mv", hi)) snprintf(t, sizeof(t), "spread %ld mV", (long)(hi - lo));
    else snprintf(t, sizeof(t), "--");
    tiles[2].setDetail(t, kit::M12);
    int64_t vbus = 0, iin = 0;
    int64_t usbRole = 0; usb.get("pd_role", usbRole);
    const UsbPortPower port = usbPortPower(usb, (int)usbRole);
    (void)iin;
    if (usb.get("tps_vbus_mv", vbus)) {
        mv(a, sizeof(a), vbus, 2);
        // Sourcing: power going out (the tile is too narrow for an estimate mark).
        if (port.valid && port.source) snprintf(t, sizeof(t), "%s V \xC2\xB7 %ld.%ld W", a, (long)(port.mw / 1000), (long)(port.mw % 1000 / 100));
        else if (port.valid) { mv(b, sizeof(b), port.ma, 2); snprintf(t, sizeof(t), "%s V \xC2\xB7 %s A", a, b); }
        else snprintf(t, sizeof(t), "%s V", a);
    }
    else snprintf(t, sizeof(t), usb.fresh ? "Detached" : "PD offline");
    tiles[3].setDetail(t, kit::M12);
    int64_t duty = 0;
    if (rail.get("duty_a_x10", duty)) snprintf(t, sizeof(t), "Buck %ld.%ld %%", (long)(duty / 10), (long)(duty % 10)); else snprintf(t, sizeof(t), "--");
    tiles[4].setDetail(t, kit::M12);
    int64_t tmax = -1000, tv;
    const char* keys[] = { "t1", "t2", "t3", "t4" };
    for (int i = 0; i < 4; ++i) if (aux.get(keys[i], tv) && tv > tmax) tmax = tv;
    if (tmax > -1000) snprintf(t, sizeof(t), "max %ld.%ld \xC2\xB0""C", (long)(tmax / 10), (long)((tmax < 0 ? -tmax : tmax) % 10)); else snprintf(t, sizeof(t), "--");
    tiles[5].setDetail(t, kit::M12);
    int64_t fault = 0;
    const bool haveFault = rail.get("fault", fault);
    const bool anyFault = s.fault_latched || (haveFault && fault);
    char issue[64];
    // Tile is narrow: symbol of the most severe fault ("FAULT OCP +1").
    char set[32] = "";
    if (fault) err::symbols(err::E_PSU, (uint32_t)fault, set, sizeof(set));
    if (set[0]) snprintf(issue, sizeof(issue), "FAULT %s", set);
    else if (s.fault_latched) { char latch[64]; snprintf(issue, sizeof(issue), "%s", s.fault[0] ? err::latchText(s.fault, latch, sizeof(latch)) : "FAULT"); }
    else strcpy(issue, "FAULT");
    tiles[6].setDetail(anyFault ? issue : rail.fresh ? "All clear" : "--", kit::M12);
    tiles[6].setInk(anyFault ? ui::NEGATIVE : rail.fresh ? ui::POSITIVE : -1);
    if (s.preset_selected) snprintf(t, sizeof(t), "P%u active", (unsigned)s.preset_selected); else snprintf(t, sizeof(t), "3 stored");
    tiles[7].setDetail(t, kit::M12);
    PsuSequencer* seq = psu_sequencer();
    const char* runNames[] = { "idle", "running", "paused", "done", "aborted" };
    snprintf(t, sizeof(t), "%u step%s · %s", (unsigned)seq->count, seq->count == 1 ? "" : "s", runNames[seq->run < 5 ? seq->run : 0]);
    tiles[8].setDetail(t, kit::M12);
    PsuCharger* chg = psu_charger();
    const char* chem[] = { "Li-ion", "LiHV", "LiFePO4", "LTO", "Lead", "NiMH", "Custom" };
    snprintf(t, sizeof(t), "%s %uS · %s", chem[chg->profile.chemistry < 7 ? chg->profile.chemistry : 0], (unsigned)chg->profile.cells, chg->running ? "charging" : "idle");
    tiles[9].setDetail(t, kit::M12);
    G4Port* g4 = psu_g4();
    snprintf(t, sizeof(t), "ACK %lu · ERR %lu", (unsigned long)g4->ack_count, (unsigned long)g4->err_count);
    tiles[10].setDetail(t, kit::M12);
    if (s.net_state == PSU_NET_BOUND) snprintf(t, sizeof(t), "%u.%u.%u.%u", s.ip[0], s.ip[1], s.ip[2], s.ip[3]);
    else snprintf(t, sizeof(t), s.ethernet_configured ? "Link not ready" : "Not configured");
    tiles[11].setDetail(t, kit::M12);
    snprintf(t, sizeof(t), "Appearance \xC2\xB7 %s", ui::Theme::themeName(ui::Theme::themeIndex()));
    tiles[12].setLabel(t);
    tiles[13].setLabel(s.service_mode ? "Service \xC2\xB7 unlocked" : "Service \xC2\xB7 locked");
}
