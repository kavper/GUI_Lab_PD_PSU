#include <gui/screenusbpd_screen/ScreenUsbPdView.hpp>
#include <gui/common/KitPage.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/UsbSession.hpp>
#include <gui/common/UsbPdObjects.hpp>
#include <stdlib.h>

namespace {
uint8_t requestedRole = 0; // 0 AUTO, 1 SINK, 2 SOURCE (last request from this panel)
uint32_t roleRequestMs = 0;

// While the port renegotiates (role swap, DRP toggling) the reported role,
// plug and capabilities can change several times a second. The page shows a
// new state only after it has been stable for a moment, so the whole page
// does not flicker between SOURCE / SINK / WAITING.
struct Steady {
    int shown, candidate; uint32_t since; bool init;
    int update(int raw, uint32_t now, uint32_t hold)
    {
        if (!init) { init = true; shown = candidate = raw; since = now; return shown; }
        if (raw == shown) { candidate = raw; return shown; }
        if (raw != candidate) { candidate = raw; since = now; }
        else if (now - since >= hold) shown = raw;
        return shown;
    }
};
Steady steadyRole = {0, 0, 0, false},
       steadyContract = {0, 0, 0, false}, steadyOnline = {0, 0, 0, false};
}

ScreenUsbPdView::ScreenUsbPdView() : divider(0), noticeTicks(0), clicked(this, &ScreenUsbPdView::onClick) { notice[0] = 0; }

void ScreenUsbPdView::setupScreen()
{
    ScreenUsbPdViewBase::setupScreen();
    layout();
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this); theme.box(LabBackground, ui::BACKGROUND); theme.apply();
    refresh();
}
void ScreenUsbPdView::tearDownScreen() { kit::app()->setSwipePage(FrontendApplication::OTHER); ScreenUsbPdViewBase::tearDownScreen(); }
void ScreenUsbPdView::navigateBack() { kit::app()->navigateBack(); }

void ScreenUsbPdView::layout()
{
    KIT_PAGE_BEGIN(FrontendApplication::USB_PD);
    portCard.setup(12, 58, 372, 308, "PORT \xC2\xB7 MEASURED");
    offersCard.setup(394, 58, 394, 308, "CHARGER OFFERS");
    flowCard.setup(12, 376, 776, 92, "BATTERY FLOW");
    add(portCard); add(offersCard); add(flowCard);
    add(contractChip);

    volts.setup(kit::M34, kit::M16, kit::LEFT, 24, 82, 176, 46, ui::VOLT);
    amps.setup(kit::M34, kit::M16, kit::LEFT, 204, 82, 176, 46, ui::AMP);
    voltsSub.setup(kit::SUB, kit::LEFT, 26, 128, 170, 16, ui::MUTED);
    ampsSub.setup(kit::SUB, kit::LEFT, 206, 128, 170, 16, ui::MUTED);
    power.setup(kit::M26, kit::M14, kit::LEFT, 24, 150, 120, 36, ui::WATT);
    powerOf.setup(kit::M16, kit::LEFT, 130, 158, 140, 24, ui::MUTED);
    percent.setup(kit::M14, kit::RIGHT, 270, 158, 102, 24, ui::MUTED);
    utilisation.setup(24, 192, 348, 8, ui::WATT);
    add(volts); add(amps); add(voltsSub); add(ampsSub); add(power); add(powerOf); add(percent); add(utilisation);
    static const char* const stats[] = { "RECEIVED", "SESSION", "PEAK IN" };
    for (int i = 0; i < 3; ++i) {
        statCaption[i].setup(kit::CAP, kit::LEFT, 26 + i * 118, 238, 110, 14, ui::MUTED);
        statCaption[i].set(stats[i]);
        statValue[i].setup(kit::M14, kit::LEFT, 26 + i * 118, 254, 114, 20, ui::TEXT);
        add(statCaption[i]); add(statValue[i]);
    }
    segment.setup(24, 306, 348, 48, ui::RAISED, ui::BORDER, ui::SURFACE, 8);
    add(segment);
    static const char* const roles[] = { "AUTO", "SINK", "SOURCE" };
    for (int i = 0; i < 3; ++i) {
        role[i].setup(28 + i * 114, 310, 112, 40, roles[i], kit::Btn::SEGMENT, kit::BODYM);
        role[i].setBase(ui::RAISED); role[i].tag = i; role[i].setAction(clicked);
        add(role[i]);
    }

    static const char* const heads[] = { "TYPE", "VOLTAGE", "", "CURRENT", "POWER" };
    static const int hx[] = { 436, 490, 0, 636, 704 }, hw[] = { 50, 110, 0, 66, 64 };
    static const kit::Align ha[] = { kit::LEFT, kit::LEFT, kit::LEFT, kit::RIGHT, kit::RIGHT };
    for (int i = 0; i < 5; ++i) {
        tableHead[i].setup(kit::CAP, ha[i], hx[i], 86, hw[i], 14, ui::MUTED);
        tableHead[i].set(heads[i]);
        add(tableHead[i]);
    }
    pdoMark.setup(404, 99, 374, 30, ui::ACCENT_SOFT, ui::ACCENT, ui::SURFACE, 6);
    add(pdoMark);
    static const int cx[] = { 412, 436, 490, 636, 704 }, cw[] = { 20, 52, 146, 66, 64 };
    static const kit::Align ca[] = { kit::LEFT, kit::LEFT, kit::LEFT, kit::RIGHT, kit::RIGHT };
    for (int r = 0; r < PDOS; ++r)
        for (int c = 0; c < 5; ++c) {
            pdoCell[r][c].setup(c == 1 ? kit::SUB : kit::M14, ca[c], cx[c], 105 + r * 31, cw[c], 28, c == 0 ? ui::MUTED : ui::TEXT);
            add(pdoCell[r][c]);
        }
    rdoLine.setup(kit::M12, kit::LEFT, 406, 334, 372, 20, ui::MUTED);
    add(rdoLine);

    flowPower.setup(kit::M26, kit::M14, kit::LEFT, 24, 400, 150, 40, ui::POSITIVE);
    flowBar.setup(190, 406, 586, 8, ui::POSITIVE, ui::RAISED);
    flowBar.setCentered(true);
    flowZero.setup(482, 400, 2, 20, ui::MUTED);
    flowLeft.setup(kit::SUB, kit::LEFT, 190, 428, 190, 16, ui::NEGATIVE);
    flowMid.setup(kit::SUB, kit::CENTER, 380, 428, 206, 16, ui::MUTED);
    flowRight.setup(kit::SUB, kit::RIGHT, 586, 428, 190, 16, ui::POSITIVE);
    add(flowPower); add(flowBar); add(flowZero); add(flowLeft); add(flowMid); add(flowRight);
    KIT_PAGE_HEADER(FrontendApplication::USB_PD);
}

void ScreenUsbPdView::onClick(const kit::Btn& b)
{
    if (b.tag == 0) roleAuto();
    else if (b.tag == 1) roleSink();
    else roleSource();
}

void ScreenUsbPdView::handleTickEvent()
{
    if (kit::app()->isScreenTransitionActive()) return;
    if (noticeTicks) --noticeTicks;
    if (++divider >= 4) { divider = 0; refresh(); }
}
void ScreenUsbPdView::notify(const char* text) { snprintf(notice, sizeof(notice), "%s", text); noticeTicks = 180; refresh(); }
void ScreenUsbPdView::allOff() { psu_app_power_shutdown(); notify("POWER OFF requested - BMS shutdown / wake with TS2"); }
// One request per press: a repeated tap within 1.5 s (or on the role that is
// already selected) is ignored, so the port is not sent back and forth.
static bool roleRequest(uint8_t role, const char* name)
{
    const uint32_t now = psu_app_now();
    if (role == requestedRole && now - roleRequestMs < 1500) return true;
    if (!psu_app_usb_role(name, PSU_SRC_LCD)) return false;
    requestedRole = role; roleRequestMs = now;
    return true;
}
void ScreenUsbPdView::roleAuto() { notify(roleRequest(0, "AUTO") ? "AUTO requested; the chip shows the actual PD contract" : "Role request blocked"); }
void ScreenUsbPdView::roleSink() { notify(roleRequest(1, "SINK") ? "SINK requested; waiting for the reported role" : "Role request blocked"); }
void ScreenUsbPdView::roleSource() { notify(roleRequest(2, "SOURCE") ? "SOURCE requested; waiting for the reported role" : "Role request blocked"); }
void ScreenUsbPdView::refreshTelemetry() { notify(g4_simple(psu_g4(), "STATUS", 0) ? "PING sent - telemetry streams automatically" : "Command queue full"); }
void ScreenUsbPdView::saveSwipeNotice(char* text, uint16_t& ticks) const { memcpy(text, notice, sizeof(notice)); ticks = noticeTicks; }
void ScreenUsbPdView::restoreSwipeNotice(const char* text, uint16_t ticks) { memcpy(notice, text, sizeof(notice)); noticeTicks = ticks; refresh(); }

void ScreenUsbPdView::refresh()
{
    TelemetryRecord usb(G4_RECORD_TC), battery(G4_RECORD_TB);
    char b[96], c[48];
    int64_t contractRole = 0, roleCode = 0, pdMv = 0, pdMa = 0, vbus = 0, iin = 0, generation = 0;
    const uint32_t now = psu_app_now();
    int64_t connectionState = 0;
    const bool haveConnectionState = usb.get("conn", connectionState);
    const bool cableOnly = haveConnectionState && connectionState == 4;
    const bool rawAttached = usb.is("plug", 1) && haveConnectionState &&
                             (connectionState == 6 || connectionState == 7);
    usb.get("pd_role", contractRole);
    roleCode = contractRole;
    if (roleCode != 1 && roleCode != 2) usb.get("role", roleCode);
    const bool rawContract = rawAttached && (contractRole == 1 || contractRole == 2) && usb.get("pd_mv", pdMv) && usb.get("pd_ma", pdMa) && pdMv > 0 && pdMa > 0;
    // A reported detach/stale frame invalidates the session immediately.
    // Smooth labels only; never hold old capabilities across a cable change.
    const bool attached = rawAttached;
    const unsigned sessionRole = (unsigned)roleCode;
    roleCode = steadyRole.update((int)roleCode, now, 1200);
    const bool contract = steadyContract.update(rawContract, now, 1200) != 0 && pdMv > 0 && pdMa > 0;
    // Buttons stay usable through short telemetry gaps.
    const bool online = steadyOnline.update(usb.record.valid && now - usb.record.ms < 3000, now, 1500) != 0;

    G4PdCapabilities caps; g4_pd_capabilities_snapshot(psu_g4(), &caps);
    usb.get("attach_gen", generation);
    const bool currentCaps = ui::pdCapabilitiesCurrent(caps, now, attached, sessionRole, (unsigned)generation);
    const unsigned selected = currentCaps ? ui::selectedPdObject(caps) : 0;

    if (!online) contractChip.set("OFFLINE", -1, ui::MUTED);
    else if (!attached) contractChip.set(cableOnly ? "CABLE ONLY" : "DETACHED", -1, ui::MUTED);
    else {
        if (selected) snprintf(b, sizeof(b), "%s \xC2\xB7 PDO %u", roleCode == 2 ? "SOURCE" : "SINK", selected);
        else snprintf(b, sizeof(b), "%s", roleCode == 1 ? "SINK" : roleCode == 2 ? "SOURCE" : "WAITING");
        contractChip.set(b, -1, ui::ACCENT);
    }
    if (contractChip.getX() != 372 - contractChip.getWidth()) { contractChip.invalidate(); contractChip.setXY(372 - contractChip.getWidth(), 64); contractChip.invalidate(); }

    // Measured port values (input ADC only measures the sink direction).
    const bool haveVbus = usb.get("tps_vbus_mv", vbus);
    if (haveVbus) { kit::milli(b, sizeof(b), vbus, "V", 2); volts.set(b, ui::VOLT); } else volts.set("-- V", ui::MUTED);
    const UsbPortPower port = attached ? usbPortPower(usb, (int)roleCode) : UsbPortPower();
    const bool powerValid = attached && port.valid;
    (void)iin;
    if (powerValid && (port.ma || !port.source)) { kit::milli(b, sizeof(b), port.ma, "A"); if (port.estimated) { memmove(b + 1, b, strlen(b) + 1); b[0] = '~'; } amps.set(b, ui::AMP); }
    else amps.set("-- A", ui::MUTED);
    if (contract) { kit::milli(c, sizeof(c), pdMv, "V", 2); snprintf(b, sizeof(b), "contract %s", c); } else snprintf(b, sizeof(b), attached ? "no PD contract" : "no partner");
    voltsSub.set(b);
    if (contract) { kit::milli(c, sizeof(c), pdMa, "A", 2); snprintf(b, sizeof(b), roleCode == 2 ? "max %s \xC2\xB7 from battery" : "limit %s", c); } else snprintf(b, sizeof(b), "limit --");
    ampsSub.set(b);
    const int64_t mw = port.mw;
    if (powerValid) { snprintf(b, sizeof(b), "%ld.%01ld W", (long)(mw / 1000), (long)(mw % 1000 / 100)); power.set(b, ui::WATT); }
    else power.set("-- W", ui::MUTED);
    const int64_t limitMw = contract ? pdMv * pdMa / 1000 : 0;
    if (limitMw) { snprintf(b, sizeof(b), "of %ld W", (long)((limitMw + 500) / 1000)); powerOf.set(b); } else powerOf.set("");
    const int pct = powerValid && limitMw ? (int)(mw * 100 / limitMw) : 0;
    if (powerValid && limitMw) { snprintf(b, sizeof(b), "%d %%", pct); percent.set(b); } else percent.set("");
    utilisation.set(pct * 10, pct > 100 ? ui::CAUTION : ui::WATT);

    // Session statistics follow the direction of the port.
    const bool sourcing = roleCode == 2;
    statCaption[0].set(sourcing ? "DELIVERED" : "RECEIVED");
    statCaption[2].set(sourcing ? "PEAK OUT" : "PEAK IN");
    const UsbSession& session = usbSession();
    if (session.hasSamples()) {
        const uint64_t cwh = session.energyMwMs / 36000000ULL;
        snprintf(b, sizeof(b), "%lu.%02lu Wh", (unsigned long)(cwh / 100), (unsigned long)(cwh % 100)); statValue[0].set(b, ui::TEXT);
        snprintf(b, sizeof(b), "%lu:%02lu:%02lu", (unsigned long)(session.elapsedMs / 3600000), (unsigned long)(session.elapsedMs / 60000 % 60), (unsigned long)(session.elapsedMs / 1000 % 60)); statValue[1].set(b, ui::TEXT);
        snprintf(b, sizeof(b), "%lu.%02lu W", (unsigned long)(session.peakMw / 1000), (unsigned long)(session.peakMw % 1000 / 10)); statValue[2].set(b, ui::TEXT);
    } else for (int i = 0; i < 3; ++i) statValue[i].set("--", ui::MUTED);
    for (int i = 0; i < 3; ++i) { role[i].setSelected(requestedRole == i); role[i].setEnabled(online); }

    // PDO table.
    unsigned rows = 0;
    if (currentCaps && (caps.flags & 2u) && caps.count) rows = caps.count > PDOS ? PDOS : caps.count;
    snprintf(b, sizeof(b), rows ? "%s \xC2\xB7 %u PDO" : "%s", currentCaps && (caps.flags & 8u) ? "ADVERTISED SOURCE PDO" : "CHARGER OFFERS", rows);
    offersCard.setCaption(b);
    for (unsigned r = 0; r < PDOS; ++r) {
        if (r >= rows) {
            for (int k = 0; k < 5; ++k) pdoCell[r][k].set("");
            continue;
        }
        const ui::PdObject p = ui::PdObject::decode(caps.pdo[r]);
        const bool sel = selected == r + 1;
        snprintf(b, sizeof(b), "%u", r + 1); pdoCell[r][0].set(b, sel ? ui::ACCENT : ui::MUTED);
        const char* type = p.type == 0 ? "Fixed" : p.type == 1 ? "Batt" : p.type == 2 ? "Var" : p.apdo == 0 ? "PPS" : p.apdo == 2 ? "AVS" : "?";
        pdoCell[r][1].set(type, sel ? ui::ACCENT : ui::MUTED);
        uint32_t maxMw = p.mw;
        if (p.valid && p.type == 0) { kit::milli(b, sizeof(b), p.maxMv, "V", 2); maxMw = p.maxMv * p.ma / 1000; }
        else if (p.valid) {
            snprintf(b, sizeof(b), "%lu.%01lu-%lu.%01lu V", (unsigned long)(p.minMv / 1000), (unsigned long)(p.minMv % 1000 / 100), (unsigned long)(p.maxMv / 1000), (unsigned long)(p.maxMv % 1000 / 100));
            if (p.type != 1) maxMw = p.maxMv * p.ma / 1000;
        } else if (p.type == 3 && p.apdo == 2) snprintf(b, sizeof(b), "9-20 V");
        else snprintf(b, sizeof(b), "0x%08lX", (unsigned long)caps.pdo[r]);
        pdoCell[r][2].set(b, ui::TEXT);
        if (p.valid && p.type != 1) kit::milli(b, sizeof(b), p.ma, "A", 2); else if (p.type == 3 && p.apdo == 2) snprintf(b, sizeof(b), "%lu.%02lu A", (unsigned long)(((caps.pdo[r] >> 10) & 1023u) / 100), (unsigned long)(((caps.pdo[r] >> 10) & 1023u) % 100)); else b[0] = 0;
        pdoCell[r][3].set(b, ui::TEXT);
        if (maxMw) snprintf(b, sizeof(b), "%lu W", (unsigned long)((maxMw + 500) / 1000)); else b[0] = 0;
        pdoCell[r][4].set(b, ui::TEXT);
    }
    // Centred on the row's digits (the text box starts at the top of the cell).
    if (selected && selected <= PDOS) {
        const int y = 98 + (int)(selected - 1) * 31;
        if (!pdoMark.isVisible() || pdoMark.getY() != y) { pdoMark.invalidate(); pdoMark.setVisible(true); pdoMark.moveTo(404, y); pdoMark.invalidate(); }
    }
    else if (pdoMark.isVisible()) { pdoMark.setVisible(false); pdoMark.invalidate(); }
    if (!rows) {
        pdoCell[0][2].set(!attached ? (cableOnly ? "Cable only / no partner" : "Connect a USB-C partner") : !caps.valid ? "PDO list not reported" : "Waiting for source PDOs", ui::MUTED);
    }
    if (currentCaps && (caps.flags & 4u)) snprintf(b, sizeof(b), "RDO 0x%08lX \xC2\xB7 contract utilisation %d %%", (unsigned long)caps.rdo, pct);
    else snprintf(b, sizeof(b), attached ? "No active RDO" : "PDOs belong to the current cable session");
    rdoLine.set(b);

    // Battery power flow gauge.
    battery.fresh = battery.fresh && battery.is("sample", 1) && battery.is("bms", 1);
    int64_t pmv = 0, pma = 0;
    const bool gaugeValid = battery.get("pack_mv", pmv) && battery.get("i_pack_ma", pma) && pmv >= 0 && pmv <= 100000 && pma >= -100000 && pma <= 100000;
    if (gaugeValid) {
        const int64_t fmw = pmv * pma / 1000, mag = fmw < 0 ? -fmw : fmw;
        gaugeScale.update(fmw, psu_app_now(), true);
        snprintf(b, sizeof(b), "%s%ld.%02ld W", fmw < 0 ? "-" : fmw > 0 ? "+" : "", (long)(mag / 1000), (long)(mag % 1000 / 10));
        const ui::Role r = pma > 0 ? ui::POSITIVE : pma < 0 ? ui::NEGATIVE : ui::TEXT;
        flowPower.set(b, r);
        flowBar.set((int)(fmw * 1000 / (int64_t)gaugeScale.range()), pma < 0 ? ui::NEGATIVE : ui::POSITIVE);
        char v[24], a[24]; kit::milli(v, sizeof(v), pmv, "V", 2); kit::milli(a, sizeof(a), pma, "A", 2);
        snprintf(b, sizeof(b), "%s \xC2\xB7 %s \xC2\xB7 auto \xC2\xB1%lu W", v, a, (unsigned long)(gaugeScale.range() / 1000));
        flowMid.set(b);
    } else {
        gaugeScale.update(0, psu_app_now(), false);
        flowPower.set("-- W", ui::MUTED); flowBar.set(0, ui::MUTED);
        flowMid.set("Waiting for valid BMS sample");
    }
    snprintf(b, sizeof(b), "-%lu W discharging", (unsigned long)(gaugeScale.range() / 1000)); flowLeft.set(b);
    snprintf(b, sizeof(b), "charging +%lu W", (unsigned long)(gaugeScale.range() / 1000)); flowRight.set(b);

    if (noticeTicks) header.setSub(notice, ui::TEXT);
    else if (!usb.record.valid) header.setSub("Waiting for PD telemetry", ui::CAUTION);
    else { snprintf(b, sizeof(b), "%s %lu ms ago", usb.fresh ? "updated" : "STALE", (unsigned long)(psu_app_now() - usb.record.ms)); header.setSub(b, usb.fresh ? ui::MUTED : ui::CAUTION); }
}
