#include <gui/common/UiTheme.hpp>
#include <gui/common/ErrorCatalog.hpp>
#include <gui/screenbattery_screen/ScreenBatteryView.hpp>
#include <gui/common/LabText.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <images/BitmapDatabase.hpp>

namespace {
FrontendApplication* app() { return static_cast<FrontendApplication*>(touchgfx::Application::getInstance()); }
void deciC(char* out, size_t n, int64_t dK)
{
    const int64_t deci = dK - 2732, mag = deci < 0 ? -deci : deci;
    snprintf(out, n, "%s%ld.%ld \xC2\xB0""C", deci < 0 ? "-" : "", (long)(mag / 10), (long)(mag % 10));
}
}

ScreenBatteryView::ScreenBatteryView() : page(0), divider(0), noticeTicks(0) { notice[0] = 0; }

void ScreenBatteryView::setupScreen()
{
    ScreenBatteryViewBase::setupScreen();
    app()->setSwipePage(FrontendApplication::BATTERY);
    layout();
    setupTheme();
    refresh();
}

void ScreenBatteryView::layout()
{
    using kit::place;
    kit::clearNoSwipe();
    kit::hide(LabHeader); kit::hide(ThemeHeaderDivider); kit::hide(PageTitle); kit::hide(PageFeedback);
    kit::hide(AllOffButton); kit::hide(BackButton); kit::hide(DetailsTab); kit::hide(UsbButton);
    kit::hide(SummaryCard); kit::hide(ChargerPanel); kit::hide(EnergyPanel); kit::hide(PackSoc);
    kit::hide(CellsHeading); kit::hide(StatusHint); kit::hide(ChargerStatus); kit::hide(EnergyHint);
    touchgfx::Box* cards[] = { &CellCard0, &CellCard1, &CellCard2, &CellCard3, &CellCard4 };
    touchgfx::TextArea* labels[] = { &CellLabel0, &CellLabel1, &CellLabel2, &CellLabel3, &CellLabel4 };
    touchgfx::TextArea* values[] = { &CellValue0, &CellValue1, &CellValue2, &CellValue3, &CellValue4 };
    touchgfx::TextArea* states[] = { &CellStatus0, &CellStatus1, &CellStatus2, &CellStatus3, &CellStatus4 };

    socCard.setup(12, 58, 286, 150); flowCard.setup(308, 58, 250, 150); healthCard.setup(568, 58, 220, 150);
    cellsCard.setup(12, 218, 546, 250); chargerCard.setup(568, 218, 220, 150); energyCard.setup(568, 378, 220, 90);
    insert(&LabBackground, energyCard); insert(&LabBackground, chargerCard); insert(&LabBackground, cellsCard);
    insert(&LabBackground, healthCard); insert(&LabBackground, flowCard); insert(&LabBackground, socCard);

    socCaption.setup(kit::CAP, kit::LEFT, 26, 68, 160, 14, ui::MUTED); socCaption.set("STATE OF CHARGE");
    place(FlowState, 150, 64, 136, 20); kit::retype(FlowState, kit::BODYM, kit::RIGHT);
    socValue.setup(kit::M52, kit::M26, kit::LEFT, 22, 88, 270, 62, ui::TEXT);
    socBar.setup(26, 154, 258, 12, 10);
    socDetail.setup(kit::SUB, kit::LEFT, 26, 176, 266, 18, ui::MUTED);
    add(socCaption); add(socValue); add(socBar); add(socDetail);

    flowCaption.setup(kit::CAP, kit::LEFT, 322, 68, 200, 14, ui::MUTED); flowCaption.set("PACK FLOW");
    add(flowCaption);
    kit::hide(PackPower); kit::hide(PackVoltage); kit::hide(PackCurrent);
    packPower.setup(kit::M34, kit::M20, kit::LEFT, 320, 86, 228, 50, ui::WATT);
    packVolts.setup(kit::M20, kit::M12, kit::LEFT, 322, 148, 112, 32, ui::VOLT);
    packAmps.setup(kit::M20, kit::M12, kit::LEFT, 436, 148, 116, 32, ui::AMP);
    add(packPower); add(packVolts); add(packAmps);

    healthCaption.setup(kit::CAP, kit::LEFT, 582, 68, 190, 14, ui::MUTED); healthCaption.set("HEALTH");
    add(healthCaption);
    const char* keys[] = { "SOH", "Cycles", "Cell temp", "BMS IC" };
    for (int i = 0; i < 4; ++i) {
        healthKey[i].setup(kit::BODY, kit::LEFT, 582, 88 + i * 20, 90, 20, ui::MUTED); healthKey[i].set(keys[i]);
        healthValue[i].setup(kit::M14, kit::RIGHT, 660, 88 + i * 20, 116, 20, ui::TEXT);
        add(healthKey[i]); add(healthValue[i]);
    }
    chgChip.setXY(580, 174); dsgChip.setXY(640, 174); faultChip.setXY(700, 174);
    add(chgChip); add(dsgChip); add(faultChip);

    cellsCaption.setup(kit::CAP, kit::LEFT, 26, 228, 200, 14, ui::MUTED); cellsCaption.set("CELLS");
    add(cellsCaption);
    place(CellSpread, 210, 225, 336, 20); kit::retype(CellSpread, kit::SUB, kit::RIGHT);
    for (int i = 0; i < 5; ++i) {
        const int x = 24 + i * 106;
        kit::hide(*cards[i]); kit::hide(*labels[i]);
        place(*values[i], x, 250, 94, 24); kit::retype(*values[i], kit::M16, kit::CENTER);
        cellLevel[i].setup(x + 22, 278, 50, 132);
        place(*states[i], x - 4, 414, 102, 18); kit::retype(*states[i], kit::SUB, kit::CENTER);
        cellName[i].setup(kit::CAP, kit::CENTER, x, 436, 94, 16, ui::MUTED);
        char n[4] = { 'C', char('1' + i), 0, 0 }; cellName[i].set(n);
        add(cellLevel[i]); add(cellName[i]);
    }

    chargerCaption.setup(kit::CAP, kit::LEFT, 582, 228, 100, 14, ui::MUTED); chargerCaption.set("CHARGER");
    chargerPhase.setup(kit::CAP, kit::RIGHT, 660, 228, 116, 14, ui::MUTED);
    add(chargerCaption); add(chargerPhase);
    kit::hide(ChargerData);
    chargerRows.setup(582, 246, 194, 66, 22);
    add(chargerRows);
    chargeBar.setup(582, 344, 192, 6, ui::POSITIVE);
    add(chargeBar);

    energyCaption.setup(kit::CAP, kit::LEFT, 582, 388, 100, 14, ui::MUTED); energyCaption.set("ENERGY");
    add(energyCaption);
    place(EnergyValue, 660, 384, 118, 22); kit::retype(EnergyValue, kit::M16, kit::RIGHT);
    trend.setPosition(580, 412, 196, 46);
    add(trend);

    header.setup(FrontendApplication::pageInfo(FrontendApplication::BATTERY));
    add(header);
}

void ScreenBatteryView::tearDownScreen() { ScreenBatteryViewBase::tearDownScreen(); }
void ScreenBatteryView::handleTickEvent()
{
    if (app()->isScreenTransitionActive()) return;
    
    if (noticeTicks) --noticeTicks;
    if (++divider >= 6) { divider = 0; refresh(); }
}
void ScreenBatteryView::notify(const char* text) { snprintf(notice, sizeof(notice), "%s", text); noticeTicks = 180; refresh(); }
void ScreenBatteryView::allOff() { notify(psu_app_power_shutdown() ? "POWER OFF requested / wake with TS2" : "POWER OFF request failed"); }
void ScreenBatteryView::showBattery() { refresh(); }
void ScreenBatteryView::showCharger() { refresh(); }
void ScreenBatteryView::showEnergy() { refresh(); }
void ScreenBatteryView::openBms() { app()->go(FrontendApplication::BMS); }
void ScreenBatteryView::openUsb() { app()->go(FrontendApplication::USB_PD); }
void ScreenBatteryView::refreshTelemetry() { notify(g4_simple(psu_g4(), "STATUS", 0) ? "Telemetry requested" : "Command queue full"); }

void ScreenBatteryView::refresh()
{
    TelemetryRecord pack(G4_RECORD_TB);
    const bool fresh = pack.fresh;
    pack.fresh = pack.is("bms", 1) && pack.is("sample", 1);
    int64_t mv = 0, ma = 0;
    const bool valid = pack.get("pack_mv", mv) && pack.get("i_pack_ma", ma) && mv >= 0 && mv <= 100000 && ma >= -100000 && ma <= 100000;
    char value[100];
    lab_show(FlowState, FlowStateBuffer, FLOWSTATE_SIZE, !valid ? "No sample" : ma < 0 ? "Discharging" : ma > 0 ? "Charging" : "Idle",
             ui::Theme::color(!valid ? ui::MUTED : ma > 0 ? ui::POSITIVE : ma < 0 ? ui::CAUTION : ui::TEXT));
    const int64_t mw = valid ? mv * ma / 1000 : 0, absMw = mw < 0 ? -mw : mw;
    if (valid) snprintf(value, sizeof(value), "%s%ld.%02ld W", mw < 0 ? "-" : mw > 0 ? "+" : "", (long)(absMw / 1000), (long)(absMw % 1000 / 10));
    else strcpy(value, "-- W");
    packPower.set(value, valid ? ui::WATT : ui::MUTED);
    pack.value(value, sizeof(value), "pack_mv", " V", 1000, 3);
    packVolts.set(value, valid ? ui::VOLT : ui::MUTED);
    pack.value(value, sizeof(value), "i_pack_ma", " A", 1000, 3);
    if (valid && ma > 0) { memmove(value + 1, value, strlen(value) + 1); value[0] = '+'; }
    packAmps.set(value, valid ? ui::AMP : ui::MUTED);

    // Gauge (G4 BATT group): SOC 0.1 %, learned capacity, SOH, cycles, time estimates.
    TelemetryRecord gauge(G4_RECORD_BATT);
    int64_t soc = 0, gflags = 0, cyc = 0, rem = 0, fcc = 0, tte = 0, ttf = 0, soh = 0;
    const bool haveSoc = gauge.get("soc_permille", soc) || pack.get("soc_permille", soc);
    gauge.get("soc_flags", gflags);
    if (haveSoc) {
        snprintf(value, sizeof(value), "%s%ld.%ld %%", (gflags & 2) ? "~" : "", (long)(soc / 10), (long)(soc % 10));
        socValue.set(value, ui::TEXT);
        socBar.set((int)soc, soc < 150 ? ui::NEGATIVE : soc < 300 ? ui::CAUTION : ui::POSITIVE);
    } else {
        socValue.set("-- %", ui::MUTED);
        socBar.set(0, ui::MUTED);
    }
    char t[24] = "";
    if (gauge.get("tte_min", tte) && ma < 0) snprintf(t, sizeof(t), " · empty in %ldh%02ld", (long)(tte / 60), (long)(tte % 60));
    else if (gauge.get("ttf_min", ttf) && ma > 0) snprintf(t, sizeof(t), " · full in %ldh%02ld", (long)(ttf / 60), (long)(ttf % 60));
    if (gauge.get("rem_mah", rem) && gauge.get("fcc_mah", fcc))
        snprintf(value, sizeof(value), "%ld.%02ld / %ld.%02ld Ah%s", (long)(rem / 1000), (long)(rem % 1000 / 10), (long)(fcc / 1000), (long)(fcc % 1000 / 10), t);
    else snprintf(value, sizeof(value), "%s", fresh ? "Capacity not learned yet" : "BMS offline");
    socDetail.set(value, ui::MUTED);

    // Health column.
    char soh_s[24]; gauge.value(soh_s, sizeof(soh_s), "soh_permille", " %", 10, 1); (void)soh;
    healthValue[0].set(soh_s, ui::TEXT);
    if (gauge.get("cycles_x100", cyc)) snprintf(value, sizeof(value), "%ld.%02ld", (long)(cyc / 100), (long)(cyc % 100)); else strcpy(value, "--");
    healthValue[1].set(value, ui::TEXT);
    int64_t temp = 0;
    if (pack.get("ts1_dK", temp) && temp >= 2000 && temp <= 4500) deciC(value, sizeof(value), temp); else strcpy(value, "--");
    healthValue[2].set(value, ui::TEXT);
    if (pack.get("int_temp_dK", temp) && temp >= 2000 && temp <= 4500) deciC(value, sizeof(value), temp); else strcpy(value, "--");
    healthValue[3].set(value, ui::TEXT);
    const bool chg = pack.is("chg", 1), dsg = pack.is("dsg", 1);
    chgChip.set("CHG", !pack.fresh ? ui::MUTED : chg ? ui::POSITIVE : ui::NEGATIVE);
    dsgChip.set("DSG", !pack.fresh ? ui::MUTED : dsg ? ui::POSITIVE : ui::NEGATIVE);
    dsgChip.setX(chgChip.getX() + chgChip.getWidth() + 6);
    char issue[64];
    const int severity = err::bmsIssue(pack, issue, sizeof(issue));
    if (!fresh) faultChip.set("OFFLINE", -1, ui::MUTED);
    else if (severity >= err::ES_FAULT) faultChip.set("FAULT", ui::NEGATIVE, ui::NEGATIVE);
    else if (severity >= err::ES_WARNING) faultChip.set("WARN", ui::CAUTION, ui::CAUTION);
    else faultChip.set("OK", -1, ui::POSITIVE);
    faultChip.setX(dsgChip.getX() + dsgChip.getWidth() + 6);
    healthCard.invalidate();

    // Cells: value, level tube (2.5 .. 4.25 V), state and balancing.
    touchgfx::TextArea* readings[] = { &CellValue0, &CellValue1, &CellValue2, &CellValue3, &CellValue4 };
    touchgfx::TextArea* states[] = { &CellStatus0, &CellStatus1, &CellStatus2, &CellStatus3, &CellStatus4 };
    touchgfx::Unicode::UnicodeChar* rb[] = { CellValue0Buffer, CellValue1Buffer, CellValue2Buffer, CellValue3Buffer, CellValue4Buffer };
    touchgfx::Unicode::UnicodeChar* sb[] = { CellStatus0Buffer, CellStatus1Buffer, CellStatus2Buffer, CellStatus3Buffer, CellStatus4Buffer };
    int64_t cells[5] = { 0 }, balance = 0; bool available[5] = { false }; int64_t lo = 100000, hi = 0, sum = 0; unsigned active = 0;
    pack.get("balance_mask", balance);
    for (unsigned i = 0; i < 5; i++) {
        char key[12]; snprintf(key, sizeof(key), "c%u_mv", i + 1);
        available[i] = pack.get(key, cells[i]);
        if (available[i] && cells[i] > 0 && cells[i] <= 5000) { if (cells[i] < lo) lo = cells[i]; if (cells[i] > hi) hi = cells[i]; sum += cells[i]; ++active; }
    }
    for (unsigned i = 0; i < 5; i++) {
        const bool usable = available[i] && cells[i] > 0 && cells[i] <= 5000;
        const bool unused = available[i] && cells[i] == -1;
        const bool balancing = usable && (balance & (1U << i));
        if (usable) snprintf(value, sizeof(value), "%ld.%03ld", (long)(cells[i] / 1000), (long)(cells[i] % 1000)); else strcpy(value, unused ? "UNUSED" : "--");
        lab_show(*readings[i], rb[i], 100, value, ui::Theme::color(usable ? ui::TEXT : ui::MUTED));
        const char* status = unused ? "inactive" : !usable ? "no data" : balancing ? "balancing" : (cells[i] == lo && cells[i] == hi) ? "" : cells[i] == lo ? "min" : cells[i] == hi ? "max" : "";
        lab_show(*states[i], sb[i], 100, status, ui::Theme::color(balancing ? ui::POSITIVE : ui::MUTED));
        const int permille = usable ? (int)((cells[i] - 2500) * 1000 / 1750) : 0;
        cellLevel[i].set(permille, ui::VOLT, balancing ? ui::POSITIVE : ui::BORDER);
    }
    if (active) snprintf(value, sizeof(value), "spread %ld mV · avg %ld.%03ld V · %u active", (long)(hi - lo), (long)(sum / active / 1000), (long)(sum / active % 1000), active);
    else strcpy(value, "waiting for valid cell samples");
    lab_show(CellSpread, CellSpreadBuffer, CELLSPREAD_SIZE, value, lab_muted());
    int64_t series = 0; char caption[24];
    if (pack.get("series", series) && series > 0) snprintf(caption, sizeof(caption), "CELLS · %ldS", (long)series); else snprintf(caption, sizeof(caption), "CELLS");
    cellsCaption.set(caption);

    // Onboard charger (BQ).
    TelemetryRecord charger(G4_RECORD_TC);
    char inputV[24], inputI[24], targetV[24], limitI[24];
    charger.fresh = charger.fresh && charger.is("bq_ok", 1);
    charger.value(inputV, sizeof(inputV), "bq_vbus_mv", " V", 1000, 2);
    charger.value(inputI, sizeof(inputI), "bq_iin_ma", " A", 1000, 2);
    charger.value(targetV, sizeof(targetV), "bq_vreg_mv", " V", 1000, 2);
    charger.value(limitI, sizeof(limitI), "bq_ichg_set_ma", " A", 1000, 2);
    const bool fast = charger.is("bq_fast", 1), pre = charger.is("bq_pre", 1);
    const char* phase = !charger.fresh ? "OFFLINE" : charger.is("bq_otg", 1) ? "USB SOURCE" : fast ? "FAST" : pre ? "PRECHARGE" : charger.is("bq_in", 1) ? "INPUT READY" : "NO INPUT";
    chargerPhase.set(phase, !charger.fresh ? ui::MUTED : (fast || pre) ? ui::POSITIVE : ui::MUTED);
    char details[180];
    int64_t ichg = 0, iset = 0;
    snprintf(details, sizeof(details), "USB in %s · %s\nTarget %s · %s", inputV, inputI, targetV, limitI);
    {
        char rows[200], v1[48];
        rows[0] = 0;
        snprintf(v1, sizeof(v1), "%s \xC2\xB7 %s", inputV, inputI); TelemetryRecord::append(rows, sizeof(rows), "USB in", v1);
        snprintf(v1, sizeof(v1), "%s \xC2\xB7 %s", targetV, limitI); TelemetryRecord::append(rows, sizeof(rows), "Target", v1);
        TelemetryRecord::append(rows, sizeof(rows), "Phase", phase);
        chargerRows.set(rows, charger.fresh);
    }
    (void)details;
    if (charger.get("bq_ichg_ma", ichg) && charger.get("bq_ichg_set_ma", iset) && iset > 0) chargeBar.set((int)(ichg * 1000 / iset), ui::POSITIVE);
    else chargeBar.set(0, ui::MUTED);

    PsuSnapshot snap; psu_snapshot(&snap);
    if (snap.battery_energy_valid) snprintf(value, sizeof(value), "%+.2f Wh", (double)snap.battery_energy_uwms / 3600000000000.0); else strcpy(value, "-- Wh");
    lab_show(EnergyValue, EnergyValueBuffer, ENERGYVALUE_SIZE, value, ui::Theme::color(ui::WATT));

    if (noticeTicks) header.setSub(notice, ui::TEXT);
    else if (fresh && severity >= err::ES_WARNING) header.setSub(issue, severity >= err::ES_FAULT ? ui::NEGATIVE : ui::CAUTION); else header.setSub(!fresh ? "BMS offline / stale telemetry" : !valid ? "BMS detected / no valid ADC sample" : 0, !fresh ? ui::CAUTION : ui::MUTED);
    trend.invalidate();
    ui::ThemeScreen::get().sync();
}

void ScreenBatteryView::setupTheme()
{
    auto& theme = ui::ThemeScreen::get(); theme.begin(*this);
    theme.box(LabBackground, ui::BACKGROUND);
    theme.apply();
}
