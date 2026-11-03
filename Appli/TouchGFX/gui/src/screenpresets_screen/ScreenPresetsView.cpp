#include <gui/screenpresets_screen/ScreenPresetsView.hpp>
#include <gui/common/KitPage.hpp>
#include <gui/common/UiTheme.hpp>
#include <gui/common/SettingsPersistence.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenPresetsView::ScreenPresetsView() : storageWatch(false), lastStorageStatus(0), selected(0), field(0), clicked(this, &ScreenPresetsView::onClick)
{
    psu_editor_clear(&editor);
    notice[0] = 0;
}

void ScreenPresetsView::setupScreen()
{
    ScreenPresetsViewBase::setupScreen();
    layout();
    ui::ThemeScreen& theme = ui::ThemeScreen::get();
    theme.begin(*this); theme.box(LabBackground, ui::BACKGROUND); theme.apply();
    setNotice("Tap a value to edit it \xC2\xB7 stored in EEPROM");
    select(0, 0);
}

void ScreenPresetsView::layout()
{
    KIT_PAGE_BEGIN(FrontendApplication::PRESETS);
    for (int i = 0; i < 3; ++i) {
        const int y = 58 + i * 90;
        row[i].setup(12, y, 528, 82, ui::SURFACE, ui::BORDER, ui::BACKGROUND, 11);
        add(row[i]);
        char n[4] = { 'P', char('1' + i), 0, 0 };
        name[i].setup(kit::M26, kit::LEFT, 26, y + 18, 60, 34, ui::TEXT);
        name[i].set(n);
        nameSub[i].setup(kit::SUB, kit::LEFT, 27, y + 52, 70, 16, ui::ACCENT);
        volt[i].setup(96, y + 10, 140, 62, "VOLTAGE", kit::Btn::GHOST, kit::CAP);
        amp[i].setup(240, y + 10, 120, 62, "LIMIT", kit::Btn::GHOST, kit::CAP);
        volt[i].tag = 10 + i; amp[i].tag = 20 + i;
        volt[i].setInk(ui::VOLT); amp[i].setInk(ui::AMP);
        maxCaption[i].setup(kit::CAP, kit::LEFT, 376, y + 22, 70, 14, ui::MUTED);
        maxCaption[i].set("MAX");
        maxValue[i].setup(kit::M16, kit::M12, kit::LEFT, 375, y + 38, 80, 24, ui::WATT);
        apply[i].setup(452, y + 20, 76, 42, "APPLY", kit::Btn::NORMAL, kit::BODYM);
        apply[i].tag = 30 + i;
        kit::Btn* b[] = { &volt[i], &amp[i], &apply[i] };
        for (kit::Btn* x : b) { x->setBase(ui::SURFACE); x->setAction(clicked); }
        add(name[i]); add(nameSub[i]); add(volt[i]); add(amp[i]); add(maxCaption[i]); add(maxValue[i]); add(apply[i]);
    }
    footnote.setup(kit::SUB, kit::LEFT, 14, 330, 520, 36, ui::MUTED);
    footnote.set("Tap a value to edit it. APPLY loads the preset to the\noutput setpoints; output state is unchanged.");
    add(footnote);

    editCard.setup(550, 58, 238, 410, "EDIT P1");
    add(editCard);
    segment.setup(562, 82, 214, 42, ui::RAISED, ui::BORDER, ui::SURFACE, 8);
    add(segment);
    fieldBtn[0].setup(565, 85, 104, 36, "VOLTAGE", kit::Btn::SEGMENT, kit::BODYM);
    fieldBtn[1].setup(669, 85, 104, 36, "LIMIT", kit::Btn::SEGMENT, kit::BODYM);
    for (int i = 0; i < 2; ++i) { fieldBtn[i].setBase(ui::RAISED); fieldBtn[i].tag = 40 + i; fieldBtn[i].setAction(clicked); add(fieldBtn[i]); }
    editValue.setup(kit::M26, kit::LEFT, 563, 132, 150, 38, ui::ACCENT);
    editRange.setup(kit::SUB, kit::RIGHT, 700, 146, 76, 16, ui::MUTED);
    add(editValue); add(editRange);
    static const char* const labels[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "" };
    for (int i = 0; i < KEYS; ++i) {
        keys[i].setup(562 + (i % 3) * 73, 178 + (i / 3) * 56, 68, 50, labels[i], kit::Btn::KEY, kit::M20);
        keys[i].setBase(ui::SURFACE); keys[i].tag = 100 + i; keys[i].setAction(clicked);
        add(keys[i]);
    }
    keys[11].setIcon(kit::ICON_DEL);
    save.setup(562, 406, 214, 50, "SAVE P1", kit::Btn::PRIMARY, kit::LABEL);
    save.setBase(ui::SURFACE); save.tag = 50; save.setAction(clicked);
    add(save);
    KIT_PAGE_HEADER(FrontendApplication::PRESETS);
}

void ScreenPresetsView::select(unsigned i, unsigned f)
{
    selected = i; field = f;
    const PsuPreset* p = psu_preset_get(i);
    psu_editor_load_milli(&editor, p ? (field ? p->current_ma : p->voltage_mv) : 0, 3);
    refresh();
}

void ScreenPresetsView::onClick(const kit::Btn& b)
{
    const int t = b.tag;
    if (t >= 10 && t < 13) select(t - 10, 0);
    else if (t >= 20 && t < 23) select(t - 20, 1);
    else if (t >= 30 && t < 33) {
        storageWatch = false;
        setNotice(psu_preset_apply(t - 30) ? "Preset requested; output state unchanged" : "Preset request blocked");
        refresh();
    } else if (t == 40 || t == 41) select(selected, t - 40);
    else if (t >= 100 && t < 111) { psu_editor_key(&editor, "123456789.0"[t - 100]); refresh(); }
    else if (t == 111) { psu_editor_backspace(&editor); refresh(); }
    else if (t == 50) {
        uint32_t v;
        if (!psu_editor_parse_milli(&editor, &v)) { psu_editor_clear(&editor); refresh(); return; }
        const uint32_t max = field ? 5000 : 27000;
        const bool clamped = v > max;
        if (clamped) v = max;
        const PsuPreset* p = psu_preset_get(selected);
        psu_preset_set(selected, field ? p->voltage_mv : v, field ? v : p->current_ma);
        storageWatch = true; lastStorageStatus = 0;
        setNotice(clamped ? "Preset limited; saving automatically" : "Preset updated; saving automatically");
        select(selected, field);
    }
}

void ScreenPresetsView::refresh()
{
    PsuSnapshot live; psu_snapshot(&live);
    char b[48];
    for (int i = 0; i < 3; ++i) {
        const PsuPreset* p = psu_preset_get(i);
        const bool editing = (unsigned)i == selected, active = live.preset_selected == i + 1;
        row[i].setBorder(editing ? ui::CAUTION : active ? ui::ACCENT : ui::BORDER);
        name[i].setRole(active ? ui::ACCENT : ui::TEXT);
        nameSub[i].set(editing ? "editing" : active ? "active" : "", editing ? ui::CAUTION : ui::ACCENT);
        if (!p) continue;
        if (editing && field == 0) snprintf(b, sizeof(b), "%s_ V", editor.text); else kit::milli(b, sizeof(b), p->voltage_mv, "V");
        volt[i].setDetail(b, kit::M20);
        if (editing && field == 1) snprintf(b, sizeof(b), "%s_ A", editor.text); else kit::milli(b, sizeof(b), p->current_ma, "A");
        amp[i].setDetail(b, kit::M20);
        volt[i].setSelected(false); amp[i].setSelected(false);
        volt[i].setTitleInk(editing && field == 0 ? ui::CAUTION : ui::MUTED);
        amp[i].setTitleInk(editing && field == 1 ? ui::CAUTION : ui::MUTED);
        const uint64_t mw = (uint64_t)p->voltage_mv * p->current_ma / 1000;
        snprintf(b, sizeof(b), "%lu.%01lu W", (unsigned long)(mw / 1000), (unsigned long)(mw % 1000 / 100));
        maxValue[i].set(b, ui::WATT);
        apply[i].setLabel(active ? "IN USE" : "APPLY");
        apply[i].setEnabled(!active);
    }
    snprintf(b, sizeof(b), "EDIT P%u", selected + 1); editCard.setCaption(b);
    snprintf(b, sizeof(b), "SAVE P%u", selected + 1); save.setLabel(b);
    fieldBtn[0].setSelected(field == 0); fieldBtn[1].setSelected(field == 1);
    snprintf(b, sizeof(b), "%s_", editor.text); editValue.set(b, field ? ui::AMP : ui::VOLT);
    editRange.set(field ? "0-5 A" : "0-27 V");
    header.setSub(notice, ui::MUTED);
}

void ScreenPresetsView::allOff() { psu_app_power_shutdown(); }

void ScreenPresetsView::handleTickEvent()
{
    ScreenPresetsViewBase::handleTickEvent();
    if (storageWatch) {
        const char* status = ui::settingsPersistenceStatus();
        if (status != lastStorageStatus) { lastStorageStatus = status; setNotice(status); refresh(); }
    }
}
