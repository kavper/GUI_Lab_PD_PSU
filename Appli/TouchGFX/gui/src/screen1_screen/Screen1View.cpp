#include <gui/common/UiTheme.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/LabText.hpp>
#include "psu_edit.h"
#include <gui/common/FrontendApplication.hpp>
#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/Color.hpp>
#include <images/BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "psu_app.h"
#include "psu_format.h"
#ifndef SIMULATOR
void PSU_SetOutputLed(uint8_t enabled);
#endif
}

Screen1View::Screen1View()
    : editTarget(EDIT_VOLTAGE),
      setVoltageMv(12000),
      currentLimitMa(2000),
      measuredCurrentMa(0),
      outputEnabled(false),
      constantCurrentMode(false),
      editLength(0),
      replaceOnNextKey(false),
      selectedPreset(0),
      editorNoticeTicks(0),
      voltageStep(1),
      currentStep(1),
      clicked(this, &Screen1View::onKit),
      stripDivider(0)
{
    editAscii[0] = '\0';
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::MAIN);
    const touchgfx::colortype light = touchgfx::Color::getColorFromRGB(23, 35, 55);
    const touchgfx::colortype dark = touchgfx::Color::getColorFromRGB(255, 255, 255);
    Key0.setLabelColor(light); Key1.setLabelColor(light); Key2.setLabelColor(light);
    Key3.setLabelColor(light); Key4.setLabelColor(light); Key5.setLabelColor(light);
    Key6.setLabelColor(light); Key7.setLabelColor(light); Key8.setLabelColor(light);
    Key9.setLabelColor(light); KeyDot.setLabelColor(light); KeyClear.setLabelColor(light);
    KeyBack.setLabelColor(light);
    Preset1Button.setLabelColor(light);
    Preset2Button.setLabelColor(light);
    Preset3Button.setLabelColor(light);
    KeyEnter.setLabelColor(dark);
    OutputLabel.setColor(light);
    psu_app_ensure();
    PsuSnapshot initial;
    psu_snapshot(&initial);
    setVoltageMv = initial.requested_mv;
    currentLimitMa = initial.requested_ma;
    updatePresetHighlight(initial.preset_selected);
    refreshSetpoints();
    selectVoltage();
#ifdef SIMULATOR
    setMeasurements(12040, 1236000, 386);
#else
    setMeasurements(0, 0, 0);
#endif
#ifdef SIMULATOR
    setInputMetrics(20000, 14900);
    setPcbTemperature(412);
#else
    setInputMetrics(0, 0);
    setPcbTemperature(0);
#endif

    layout();
    setupTheme();
    refreshRemoteSense();
    refreshPowerStrip();
}

// Unified layout (mockup "Main"): two measurement cards with the setpoint
// column, power strip and presets on the left; output switch and keypad on the
// right. Designer widgets stay as hidden state holders; kit widgets mirror them.
void Screen1View::layout()
{
    using kit::place;
    kit::clearNoSwipe();
    kit::hideAll(getRootContainer());
    Background.setVisible(true);
    // Invisible hit areas used by handleClickEvent to pick the edit target.
    place(SetVoltageButton, 12, 58, 478, 132);
    place(SetCurrentButton, 12, 200, 478, 132);

    for (int c = 0; c < 2; ++c) {
        const int y = c ? 200 : 58;
        card[c].setup(12, y, 478, 132, ui::SURFACE, ui::BORDER, ui::BACKGROUND, 11);
        add(card[c]);
        caption[c].setup(kit::CAP, kit::LEFT, 26, y + 11, 280, 16, ui::MUTED);
        caption[c].set(c ? "OUTPUT CURRENT" : "OUTPUT VOLTAGE");
        measured[c].setup(kit::M60, kit::M26, kit::LEFT, 22, y + 30, 300, 72, c ? ui::AMP : ui::VOLT);
        divider[c].setup(326, y + 14, 1, 104, ui::BORDER);
        setCaption[c].setup(kit::CAP, kit::LEFT, 342, y + 11, 136, 16, ui::MUTED);
        setCaption[c].set(c ? "LIMIT" : "SET");
        setValue[c].setup(kit::M26, kit::M14, kit::LEFT, 340, y + 32, 146, 36, ui::TEXT);
        nudge[c][0].setup(340, y + 84, 38, 36, "", kit::Btn::NORMAL);
        nudge[c][0].setIcon(kit::ICON_MINUS);
        nudge[c][1].setup(383, y + 84, 56, 36, "0.1", kit::Btn::NORMAL, kit::M12);
        nudge[c][2].setup(444, y + 84, 38, 36, "", kit::Btn::NORMAL);
        nudge[c][2].setIcon(kit::ICON_PLUS);
        add(caption[c]); add(measured[c]); add(divider[c]); add(setCaption[c]); add(setValue[c]);
        for (int k = 0; k < 3; ++k) {
            nudge[c][k].setBase(ui::SURFACE);
            nudge[c][k].tag = 10 + c * 3 + k;
            nudge[c][k].setAction(clicked);
            add(nudge[c][k]);
        }
    }
    senseLink.setup(16, 58 + 100, 240, 26, "", kit::Btn::GHOST, kit::SUB);
    senseLink.setBase(ui::SURFACE);
    senseLink.setAlign(kit::LEFT);
    senseLink.tag = 40; senseLink.setAction(clicked);
    add(senseLink);
    currentBar.setup(26, 200 + 116, 284, 5, ui::AMP);
    add(currentBar);

    // Power strip.
    powerStrip.setup(12, 342, 478, 58, ui::SURFACE, ui::BORDER, ui::BACKGROUND, 11);
    add(powerStrip);
    powerValue.setup(kit::M26, kit::M14, kit::LEFT, 24, 350, 150, 42, ui::WATT);
    modeChip.setXY(176, 359);
    loadValue.setup(kit::SUB, kit::LEFT, 236, 352, 246, 18, ui::MUTED);
    notice.setup(kit::SUB, kit::LEFT, 236, 372, 246, 18, ui::MUTED);
    add(powerValue); add(modeChip); add(loadValue); add(notice);

    // Presets.
    for (int i = 0; i < 3; ++i) {
        preset[i].setup(12 + i * 162, 410, 154, 58, "", kit::Btn::NORMAL, kit::CAP);
        preset[i].tag = 50 + i; preset[i].setAction(clicked);
        add(preset[i]);
    }

    // Output switch and keypad.
    output.setup(500, 58, 288, 76, "OUTPUT OFF", kit::Btn::NORMAL, kit::HEAD);
    output.tag = 60; output.setAction(clicked);
    add(output);
    keypad.setup(500, 144, 288, 324, ui::SURFACE, ui::BORDER, ui::BACKGROUND, 11);
    add(keypad);
    static const char* const labels[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "" };
    for (int i = 0; i < 12; ++i) {
        keys[i].setup(510 + (i % 3) * 92, 154 + (i / 3) * 62, 86, 56, labels[i], kit::Btn::KEY, kit::M26);
        keys[i].setBase(ui::SURFACE);
        keys[i].tag = 100 + i; keys[i].setAction(clicked);
        add(keys[i]);
    }
    keys[11].setIcon(kit::ICON_DEL);
    clearKey.setup(510, 402, 86, 56, "CLEAR", kit::Btn::KEY, kit::BODYM);
    enterKey.setup(602, 402, 178, 56, "SET VOLTAGE", kit::Btn::PRIMARY, kit::LABEL);
    clearKey.setBase(ui::SURFACE); enterKey.setBase(ui::SURFACE);
    clearKey.tag = 112; enterKey.tag = 113;
    clearKey.setAction(clicked); enterKey.setAction(clicked);
    add(clearKey); add(enterKey);

    header.setup(FrontendApplication::pageInfo(FrontendApplication::MAIN));
    add(header);
    syncKit();
}

void Screen1View::onKit(const kit::Btn& b)
{
    switch (b.tag) {
    case 10: nudgeVoltDown(); break;
    case 11: stepVolt(); break;
    case 12: nudgeVoltUp(); break;
    case 13: nudgeAmpDown(); break;
    case 14: stepAmp(); break;
    case 15: nudgeAmpUp(); break;
    case 40: toggleRemoteSense(); break;
    case 50: preset1(); break;
    case 51: preset2(); break;
    case 52: preset3(); break;
    case 60: outputToggled(); break;
    case 112: keyClear(); break;
    case 113: keyEnter(); break;
    default:
        if (b.tag >= 100 && b.tag < 112) {
            static const char k[] = "123456789.0";
            if (b.tag == 111) keyBack(); else appendKey(k[b.tag - 100]);
        }
        break;
    }
    syncKit();
}

namespace {
void utf8(const touchgfx::Unicode::UnicodeChar* src, char* out, size_t n)
{
    touchgfx::Unicode::toUTF8(src, reinterpret_cast<uint8_t*>(out), (uint16_t)n);
    out[n - 1] = 0;
}
bool dashes(const char* t) { return t[0] == '-' && t[1] == '-'; }
}

// Mirror the state kept in the Designer widgets into the kit widgets.
void Screen1View::syncKit()
{
    char t[48];
    utf8(ActualVoltageValueBuffer, t, sizeof(t));
    measured[0].set(t, dashes(t) ? ui::MUTED : ui::VOLT);
    utf8(ActualCurrentValueBuffer, t, sizeof(t));
    measured[1].set(t, dashes(t) ? ui::MUTED : ui::AMP);
    utf8(SetVoltageValueBuffer, t, sizeof(t));
    setValue[0].set(t, editTarget == EDIT_VOLTAGE ? ui::ACCENT : ui::TEXT);
    utf8(SetCurrentValueBuffer, t, sizeof(t));
    setValue[1].set(t, editTarget == EDIT_CURRENT ? ui::ACCENT : ui::TEXT);
    card[0].setBorder(editTarget == EDIT_VOLTAGE ? ui::ACCENT : ui::BORDER);
    card[1].setBorder(editTarget == EDIT_CURRENT ? ui::ACCENT : ui::BORDER);
    setCaption[0].setRole(editTarget == EDIT_VOLTAGE ? ui::ACCENT : ui::MUTED);
    setCaption[1].setRole(editTarget == EDIT_CURRENT ? ui::ACCENT : ui::MUTED);
    static const char* const vSteps[] = { "10 mV", "0.1 V", "1 V" };
    static const char* const aSteps[] = { "10 mA", "0.1 A", "1 A" };
    nudge[0][1].setLabel(vSteps[voltageStep % 3]);
    nudge[1][1].setLabel(aSteps[currentStep % 3]);
    currentBar.set(currentLimitMa ? (int)((int64_t)measuredCurrentMa * 1000 / currentLimitMa) : 0,
                   constantCurrentMode ? ui::CAUTION : ui::AMP);

    // Remote sense line in the voltage card.
    utf8(RemoteSenseTextBuffer, t, sizeof(t));
    char* nl = strchr(t, '\n');
    char line[48];
    snprintf(line, sizeof(line), "Remote sense: %s", nl ? nl + 1 : t);
    for (char* p = line + 14; *p; ++p) if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
    senseLink.setLabel(line);

    // Output switch.
    utf8(OutputLabelBuffer, t, sizeof(t));
    const bool on = !strcmp(t, "OUTPUT ON");
    output.setLabel(t);
    output.setStyle(on ? kit::Btn::POSITIVE_FILL : kit::Btn::NORMAL);
    output.setIcon(on ? kit::ICON_DOT : kit::ICON_NONE);
    output.setIconLeft(true);

    // Mode chip and notices.
    PsuSnapshot live;
    psu_snapshot(&live);
    const bool showMode = modeShown || (on && live.regulation_mode);
    if (showMode) modeChip.set(constantCurrentMode ? "CC" : "CV", -1, constantCurrentMode ? ui::CAUTION : ui::POSITIVE);
    else modeChip.set("");
    modeChip.setVisible(showMode);
    if (editorNoticeTicks) { utf8(EditorStatusBuffer, t, sizeof(t)); notice.set(t, ui::CAUTION); }
    else notice.set(editTarget == EDIT_VOLTAGE ? "Keypad edits the voltage" : "Keypad edits the current limit", ui::MUTED);

    // Presets.
    for (int i = 0; i < 3; ++i) {
        const PsuPreset* p = psu_preset_get(i);
        char d[40];
        if (p) snprintf(d, sizeof(d), "%lu.%02lu V \xC2\xB7 %lu.%01lu A", (unsigned long)(p->voltage_mv / 1000), (unsigned long)(p->voltage_mv % 1000 / 10),
                        (unsigned long)(p->current_ma / 1000), (unsigned long)(p->current_ma % 1000 / 100));
        else snprintf(d, sizeof(d), "--");
        snprintf(t, sizeof(t), selectedPreset == i + 1 ? "P%d \xC2\xB7 active" : "P%d", i + 1);
        preset[i].setLabel(t);
        preset[i].setDetail(d, kit::M14);
        preset[i].setSelected(selectedPreset == i + 1);
    }
    enterKey.setLabel(editTarget == EDIT_VOLTAGE ? "SET VOLTAGE" : "SET LIMIT");
}

void Screen1View::refreshPowerStrip()
{
    PsuSnapshot live;
    psu_snapshot(&live);
    char text[48];
    const bool valid = live.g0_connected && !live.g0_stale && live.current_valid;
    const int32_t ua = live.display_current_ua > 0 ? live.display_current_ua : 0;
    if (valid) {
        const uint64_t mw = (uint64_t)live.vout_mv * (uint64_t)ua / 1000000ULL;
        snprintf(text, sizeof(text), "%lu.%02lu W", (unsigned long)(mw / 1000), (unsigned long)(mw % 1000 / 10));
        powerValue.set(text, ui::WATT);
        if (ua > 5000 && live.output_confirmed) {
            const uint64_t mohm = (uint64_t)live.vout_mv * 1000000ULL / (uint64_t)ua;
            if (mohm < 1000000ULL) snprintf(text, sizeof(text), "Load %lu.%02lu \xCE\xA9", (unsigned long)(mohm / 1000), (unsigned long)(mohm % 1000 / 10));
            else snprintf(text, sizeof(text), "Load %lu \xCE\xA9", (unsigned long)(mohm / 1000));
        } else snprintf(text, sizeof(text), live.output_confirmed ? "No load" : "Output off");
        loadValue.set(text, ui::MUTED);
    } else {
        powerValue.set("-- W", ui::MUTED);
        loadValue.set(live.g0_connected ? "Waiting for current" : "LDO link offline", ui::MUTED);
    }
}

void Screen1View::tearDownScreen()
{
    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->setSwipePage(FrontendApplication::OTHER);
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    const bool onVoltageTile = editTarget == EDIT_VOLTAGE &&
        event.getX() >= 300 && event.getX() < 500 &&
        event.getY() >= 80 && event.getY() < 220;
    const bool onCurrentTile = editTarget == EDIT_CURRENT &&
        event.getX() >= 300 && event.getX() < 500 &&
        event.getY() >= 230 && event.getY() < 380;
    if (event.getType() != touchgfx::GestureEvent::SWIPE_VERTICAL ||
        (!onVoltageTile && !onCurrentTile))
    {
        Screen1ViewBase::handleGestureEvent(event);
        return;
    }

    const int speed = event.getVelocity() < 0 ? -event.getVelocity() : event.getVelocity();
    const bool increase = event.getVelocity() < 0;
    updatePresetHighlight(0);

    if (editTarget == EDIT_VOLTAGE)
    {
        uint32_t stepMv = 10U;
        if (speed > 2) stepMv = 100U;
        if (speed > 8) stepMv = 500U;
        if (increase)
            setVoltageMv = setVoltageMv <= (27000U - stepMv)
                ? setVoltageMv + stepMv : 27000U;
        else
            setVoltageMv = setVoltageMv >= stepMv
                ? setVoltageMv - stepMv : 0U;
    }
    else
    {
        uint32_t stepMa = 10U;
        if (speed > 2) stepMa = 50U;
        if (speed > 8) stepMa = 200U;
        if (increase)
            currentLimitMa = currentLimitMa <= (5000U - stepMa)
                ? currentLimitMa + stepMa : 5000U;
        else
            currentLimitMa = currentLimitMa >= stepMa
                ? currentLimitMa - stepMa : 0U;
    }

    refreshSetpoints();
    loadEditorFromSetpoint();
    replaceOnNextKey = true;
    submitSetpoints();
}

void Screen1View::handleClickEvent(const touchgfx::ClickEvent& event)
{
    if (event.getType() == touchgfx::ClickEvent::PRESSED)
    {
        if (SetVoltageButton.isTouchable() && SetVoltageButton.getRect().intersect(event.getX(), event.getY()))
            selectVoltage();
        else if (SetCurrentButton.isTouchable() && SetCurrentButton.getRect().intersect(event.getX(), event.getY()))
            selectCurrent();
        ui::ThemeScreen::get().sync();
    }
    // Selection is not repeated on release: a vertical swipe keeps its edit.
    Screen1ViewBase::handleClickEvent(event);
}

void Screen1View::setMeasurements(uint32_t voltageMv, int32_t currentUa, int16_t temperatureDeciC)
{
    char text[32];
    if (voltageMv < 10000)
        snprintf(text, sizeof(text),
                                   "%u.%03u V", voltageMv / 1000, voltageMv % 1000);
    else
        snprintf(text, sizeof(text),
                                   "%u.%02u V", voltageMv / 1000, (voltageMv % 1000) / 10);
    lab_show(ActualVoltageValue,ActualVoltageValueBuffer,ACTUALVOLTAGEVALUE_SIZE,text,lab_text());
    const int32_t displayCurrentUa = currentUa > 0 ? currentUa : 0;
    const int32_t roundedCurrentMa = (displayCurrentUa + 500) / 1000;
    snprintf(text, sizeof(text),
                               "%d.%03d A", (int)(roundedCurrentMa / 1000),
                               (int)(roundedCurrentMa % 1000));
    lab_show(ActualCurrentValue,ActualCurrentValueBuffer,ACTUALCURRENTVALUE_SIZE,text,lab_text());
    PsuSnapshot live;psu_snapshot(&live);
    if(!live.g4_uart_configured){psu_format_deci_c(text,sizeof(text),temperatureDeciC);lab_show(TemperatureValue,TemperatureValueBuffer,TEMPERATUREVALUE_SIZE,text,lab_text());}
    measuredCurrentMa = displayCurrentUa / 1000;
    setRegulationMode(outputEnabled && measuredCurrentMa >= (int32_t)currentLimitMa);
}

void Screen1View::setInputMetrics(uint32_t inputVoltageMv, uint32_t outputPowerMw)
{
    (void)inputVoltageMv;(void)outputPowerMw;
    // Header values are populated from independent atomic METER/TB records.

}

void Screen1View::setCurrentMeasurementCalibrated(bool calibrated)
{
    const touchgfx::TypedText text(calibrated
        ? T_TXT_ACTUAL_I
        : T_TXT_ACTUAL_I_UNCAL);
    if(ActualCurrentLabel.getTypedText().getId()!=text.getId()){ActualCurrentLabel.setTypedText(text);ActualCurrentLabel.invalidate();}
}

void Screen1View::setControllerOutputState(bool enabled)
{
    PsuSnapshot live;
    psu_snapshot(&live);
    outputEnabled = enabled;
    if(OutputEnable.getState()!=enabled){OutputEnable.forceState(enabled);OutputEnable.invalidate();}
    // The control label describes the action that a press will perform.  The
    // previous "PSU OFF" label described the current state and made the
    // toggle's behaviour ambiguous.
    const char *label = live.shutdown_pending ? "TURNING OFF..." : !live.g0_connected ? "NO LINK" : live.output_requested != enabled
        ? (live.output_requested ? "TURNING ON..." : "TURNING OFF...")
        : (enabled ? "OUTPUT ON" : "OUTPUT OFF");
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(label), OutputLabelBuffer, OUTPUTLABEL_SIZE);
    OutputLabel.setColor(ui::Theme::color(enabled ? ui::ON_ACCENT : ui::TEXT));
    OutputEnable.invalidate(); OutputLabel.invalidate();
#ifndef SIMULATOR
    PSU_SetOutputLed(enabled ? 1U : 0U);
#endif
}

void Screen1View::setPcbTemperature(int16_t temperatureDeciC)
{
    const int32_t absolute = temperatureDeciC < 0 ? -(int32_t)temperatureDeciC : (int32_t)temperatureDeciC;
    if (temperatureDeciC < 0)
        touchgfx::Unicode::snprintf(PcbTemperatureValueBuffer, PCBTEMPERATUREVALUE_SIZE,
                                   "-%d.%d C", absolute / 10, absolute % 10);
    else
        touchgfx::Unicode::snprintf(PcbTemperatureValueBuffer, PCBTEMPERATUREVALUE_SIZE,
                                   "%d.%d C", absolute / 10, absolute % 10);
    PcbTemperatureValue.invalidate();
}

void Screen1View::refreshSetpoints()
{
    touchgfx::Unicode::snprintf(SetVoltageValueBuffer, SETVOLTAGEVALUE_SIZE,
                               "%u.%03u V", setVoltageMv / 1000, setVoltageMv % 1000);
    touchgfx::Unicode::snprintf(SetCurrentValueBuffer, SETCURRENTVALUE_SIZE,
                               "%u.%03u A", currentLimitMa / 1000, currentLimitMa % 1000);
    SetVoltageValue.invalidate();
    SetCurrentValue.invalidate();
}

void Screen1View::loadEditorFromSetpoint()
{
    const uint32_t value = (editTarget == EDIT_VOLTAGE) ? setVoltageMv : currentLimitMa;
    const uint32_t decimals = 3;
    if (decimals == 2)
    {
        snprintf(editAscii, sizeof(editAscii), "%lu.%02lu",
                 static_cast<unsigned long>(value / 1000),
                 static_cast<unsigned long>((value % 1000) / 10));
    }
    else
    {
        snprintf(editAscii, sizeof(editAscii), "%lu.%03lu",
                 static_cast<unsigned long>(value / 1000),
                 static_cast<unsigned long>(value % 1000));
    }
    editLength = static_cast<uint8_t>(strlen(editAscii));
    refreshEditor();

}

void Screen1View::showEditorStatus(const char* text, bool warning)
{
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(text), EditorStatusBuffer, EDITORSTATUS_SIZE);
    editorNoticeTicks=180;
    EditorStatus.setVisible(false); // shown by the kit notice line
    EditorStatus.setColor(ui::Theme::color(warning ? ui::CAUTION : ui::POSITIVE));
    EditorStatus.invalidate();
}

void Screen1View::refreshEditor()
{
    editorNoticeTicks=0;
    EditorStatus.setVisible(false);
    EditorStatus.invalidate();
    char displayAscii[16];
    snprintf(displayAscii, sizeof(displayAscii), "%s %c", editAscii,
             editTarget == EDIT_VOLTAGE ? 'V' : 'A');
    if (editTarget == EDIT_VOLTAGE)
    {
        touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(displayAscii),
                                   SetVoltageValueBuffer, SETVOLTAGEVALUE_SIZE);
        SetVoltageValue.invalidate();
    }
    else
    {
        touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(displayAscii),
                                   SetCurrentValueBuffer, SETCURRENTVALUE_SIZE);
        SetCurrentValue.invalidate();
    }
}

void Screen1View::appendKey(char key)
{
    if (selectedPreset != 0)
        updatePresetHighlight(0);

    if (replaceOnNextKey)
    {
        editLength = 0;
        editAscii[0] = '\0';
        replaceOnNextKey = false;
    }
    if (editLength >= sizeof(editAscii) - 1)
        return;
    if (key == '.' && strchr(editAscii, '.') != 0)
        return;
    const char* dot = strchr(editAscii, '.');
    if (key != '.' && ((dot && strlen(dot + 1) >= 3) || (!dot && editLength >= 2)))
    {
        showEditorStatus("Max. 3 decimal places", true);
        return;
    }
    if (key != '.' && editLength == 1 && editAscii[0] == '0') editLength = 0;
    if (key == '.' && editLength == 0)
    {
        editAscii[editLength++] = '0';
    }
    editAscii[editLength++] = key;
    editAscii[editLength] = '\0';
    refreshEditor();
}

void Screen1View::selectVoltage()
{
    refreshSetpoints();
    editTarget = EDIT_VOLTAGE;
    VoltageSelection.setVisible(false); // kit card border shows the selection
    CurrentSelection.setVisible(false);
    VoltageSelection.invalidate();
    CurrentSelection.invalidate();
    SetVoltageLabel.setColor(ui::Theme::color(ui::ACCENT));
    SetVoltageValue.setColor(ui::Theme::color(ui::ACCENT));
    SetCurrentLabel.setColor(ui::Theme::color(ui::MUTED));
    SetCurrentValue.setColor(ui::Theme::color(ui::TEXT));
    SetVoltageLabel.invalidate(); SetVoltageValue.invalidate();
    SetCurrentLabel.invalidate(); SetCurrentValue.invalidate();
    loadEditorFromSetpoint();
    replaceOnNextKey = true;
}

void Screen1View::selectCurrent()
{
    refreshSetpoints();
    editTarget = EDIT_CURRENT;
    VoltageSelection.setVisible(false);
    CurrentSelection.setVisible(false);
    VoltageSelection.invalidate();
    CurrentSelection.invalidate();
    SetVoltageLabel.setColor(ui::Theme::color(ui::MUTED));
    SetVoltageValue.setColor(ui::Theme::color(ui::TEXT));
    SetCurrentLabel.setColor(ui::Theme::color(ui::ACCENT));
    SetCurrentValue.setColor(ui::Theme::color(ui::ACCENT));
    SetVoltageLabel.invalidate(); SetVoltageValue.invalidate();
    SetCurrentLabel.invalidate(); SetCurrentValue.invalidate();
    loadEditorFromSetpoint();
    replaceOnNextKey = true;
}

void Screen1View::outputToggled()
{
    PsuSnapshot live;
    psu_snapshot(&live);
    if(live.output_requested || live.psu_running || live.output_confirmed)psu_app_shutdown();
    else if(!psu_app_set_output(1, PSU_SRC_LCD)) {
        psu_snapshot(&live);
        showEditorStatus(live.last_on_reject, true);
    }
    psu_snapshot(&live);
    setControllerOutputState(live.output_confirmed != 0);
}

void Screen1View::setRegulationMode(bool constantCurrent)
{
    const auto bitmap=constantCurrent?BITMAP_MODE_CC_58X36_ID:BITMAP_MODE_CV_58X36_ID;
    if(ModePill.getBitmap().getId()==bitmap){constantCurrentMode=constantCurrent;return;}
    constantCurrentMode = constantCurrent;
    ModePill.setBitmap(touchgfx::Bitmap(constantCurrent
        ? BITMAP_MODE_CC_58X36_ID
        : BITMAP_MODE_CV_58X36_ID));
    ModeTextFront.setTypedText(touchgfx::TypedText(constantCurrent
        ? T_TXT_MODE_CC
        : T_TXT_MODE_CV));
    ModePill.invalidate();
    ModeTextFront.invalidate();
}

void Screen1View::key0() { appendKey('0'); }
void Screen1View::key1() { appendKey('1'); }
void Screen1View::key2() { appendKey('2'); }
void Screen1View::key3() { appendKey('3'); }
void Screen1View::key4() { appendKey('4'); }
void Screen1View::key5() { appendKey('5'); }
void Screen1View::key6() { appendKey('6'); }
void Screen1View::key7() { appendKey('7'); }
void Screen1View::key8() { appendKey('8'); }
void Screen1View::key9() { appendKey('9'); }
void Screen1View::keyDot() { appendKey('.'); }

void Screen1View::keyClear()
{
    updatePresetHighlight(0);
    replaceOnNextKey = false;
    editLength = 0;
    editAscii[0] = '\0';
    refreshEditor();
}

void Screen1View::keyBack()
{
    updatePresetHighlight(0);
    replaceOnNextKey = false;
    if (editLength > 0)
    {
        editAscii[--editLength] = '\0';
        refreshEditor();
    }
}

void Screen1View::keyEnter()
{
    if (editLength == 0)
    {
        showEditorStatus("Enter a value first", true);
        return;
    }
    PsuEditor input = {};
    snprintf(input.text,sizeof(input.text),"%s",editAscii);input.length=strlen(input.text);
    uint32_t entered=0;
    if(!psu_editor_parse_milli(&input,&entered)){editLength=0;editAscii[0]=0;refreshEditor();return;}
    const uint32_t maximum=editTarget==EDIT_VOLTAGE?27000U:5000U;
    if(entered>maximum)entered=maximum;
    updatePresetHighlight(0);
    if (editTarget == EDIT_VOLTAGE) setVoltageMv = entered;
    else currentLimitMa = entered;
    refreshSetpoints();
    loadEditorFromSetpoint();
    replaceOnNextKey = true;
    setRegulationMode(outputEnabled && measuredCurrentMa >= (int32_t)currentLimitMa);
    submitSetpoints();
}

void Screen1View::setLinkStatus(const char *text)
{
    lab_show(LinkStatus,LinkStatusBuffer,LINKSTATUS_SIZE,text?text:"G4 OFFLINE",text&&strstr(text,"ONLINE")?lab_green():lab_amber());
}

void Screen1View::updatePresetHighlight(uint8_t preset)
{
    selectedPreset = preset;
    {const PsuPreset* p=psu_preset_get(0);char b[40];if(p){snprintf(b,sizeof(b),"P1  %lu.%02lu V\n%lu.%03lu A",(unsigned long)(p->voltage_mv/1000),(unsigned long)(p->voltage_mv%1000/10),(unsigned long)(p->current_ma/1000),(unsigned long)(p->current_ma%1000));touchgfx::Unicode::fromUTF8((const uint8_t*)b,QuickPreset1Buffer,QUICKPRESET1_SIZE);QuickPreset1.invalidate();}}
    {const PsuPreset* p=psu_preset_get(1);char b[40];if(p){snprintf(b,sizeof(b),"P2  %lu.%02lu V\n%lu.%03lu A",(unsigned long)(p->voltage_mv/1000),(unsigned long)(p->voltage_mv%1000/10),(unsigned long)(p->current_ma/1000),(unsigned long)(p->current_ma%1000));touchgfx::Unicode::fromUTF8((const uint8_t*)b,QuickPreset2Buffer,QUICKPRESET2_SIZE);QuickPreset2.invalidate();}}
    {const PsuPreset* p=psu_preset_get(2);char b[40];if(p){snprintf(b,sizeof(b),"P3  %lu.%02lu V\n%lu.%03lu A",(unsigned long)(p->voltage_mv/1000),(unsigned long)(p->voltage_mv%1000/10),(unsigned long)(p->current_ma/1000),(unsigned long)(p->current_ma%1000));touchgfx::Unicode::fromUTF8((const uint8_t*)b,QuickPreset3Buffer,QUICKPRESET3_SIZE);QuickPreset3.invalidate();}}


    const touchgfx::Bitmap released(BITMAP_MAIN_PRESET_REL_100X60_ID);
    const touchgfx::Bitmap selected(BITMAP_MAIN_PRESET_SEL_100X60_ID);
    const touchgfx::colortype normalText = touchgfx::Color::getColorFromRGB(23, 35, 55);
    const touchgfx::colortype selectedText = touchgfx::Color::getColorFromRGB(36, 87, 230);

    Preset1Button.setBitmaps(preset == 1 ? selected : released, selected);
    Preset2Button.setBitmaps(preset == 2 ? selected : released, selected);
    Preset3Button.setBitmaps(preset == 3 ? selected : released, selected);
    Preset1Button.setLabelColor(preset == 1 ? selectedText : normalText);
    Preset2Button.setLabelColor(preset == 2 ? selectedText : normalText);
    Preset3Button.setLabelColor(preset == 3 ? selectedText : normalText);
    Preset1Button.invalidate();
    Preset2Button.invalidate();
    Preset3Button.invalidate();
    touchgfx::TextArea* quick[] = { &QuickPreset1, &QuickPreset2, &QuickPreset3 };
    for (int i = 0; i < 3; ++i) { quick[i]->setColor(kit::col(preset == i + 1 ? ui::ACCENT : ui::TEXT)); quick[i]->invalidate(); }
}

void Screen1View::applyPreset(uint8_t preset, uint32_t voltageMv, uint32_t currentMa)
{
    setVoltageMv = voltageMv;
    currentLimitMa = currentMa;
    updatePresetHighlight(preset);
    refreshSetpoints();
    loadEditorFromSetpoint();
    replaceOnNextKey = true;
    setRegulationMode(outputEnabled && measuredCurrentMa >= (int32_t)currentLimitMa);
    submitSetpoints();
}

void Screen1View::preset1() { const PsuPreset* p=psu_preset_get(0);if(p)applyPreset(1,p->voltage_mv,p->current_ma); }
void Screen1View::preset2() { const PsuPreset* p=psu_preset_get(1);if(p)applyPreset(2,p->voltage_mv,p->current_ma); }
void Screen1View::preset3() { const PsuPreset* p=psu_preset_get(2);if(p)applyPreset(3,p->voltage_mv,p->current_ma); }

void Screen1View::submitSetpoints()
{
    const uint32_t requestedMv = setVoltageMv;
    const uint32_t requestedMa = currentLimitMa;
    const bool accepted = psu_app_set_limits(requestedMv, requestedMa, PSU_SRC_LCD) != 0;
    PsuSnapshot current;
    psu_snapshot(&current);
    setVoltageMv = current.requested_mv;
    currentLimitMa = current.requested_ma;
    const bool limited = setVoltageMv != requestedMv || currentLimitMa != requestedMa;
    if (!accepted || limited) updatePresetHighlight(0);
    refreshSetpoints();
    loadEditorFromSetpoint();
    replaceOnNextKey = true;
    showEditorStatus(!accepted ? (current.fault_latched ? current.fault :
                     "SET TRANSPORT BUSY") :
                     limited ? "Limited by protection" : "Setpoint requested", !accepted || limited);
}

void Screen1View::setTelemetryAvailable(bool available, bool currentValid, bool temperatureValid)
{
    PsuSnapshot live;
    psu_snapshot(&live);
    const bool showMode = available && live.regulation_mode != 0;
    if (modeShown != showMode) {
        ModeTextFront.setVisible(false); ModePill.setVisible(false); modeShown = showMode;
        ModeTextFront.invalidate(); ModePill.invalidate();
    }
    if (!available) {
        lab_show(ActualVoltageValue,ActualVoltageValueBuffer,ACTUALVOLTAGEVALUE_SIZE,"-- V",lab_muted());
    }
    if (!live.g4_uart_configured && (!available || !temperatureValid)) {
        lab_show(TemperatureValue,TemperatureValueBuffer,TEMPERATUREVALUE_SIZE,"-- C",lab_muted());
        lab_show(PcbTemperatureValue,PcbTemperatureValueBuffer,PCBTEMPERATUREVALUE_SIZE,"-- C",lab_muted());
    }
    if (!available || !currentValid) {
        lab_show(ActualCurrentValue,ActualCurrentValueBuffer,ACTUALCURRENTVALUE_SIZE,"-- A",lab_muted());
        if(!live.g4_uart_configured)lab_show(PowerValue,PowerValueBuffer,POWERVALUE_SIZE,"-- W",lab_muted());
    }
    ActualVoltageValue.setColor(ui::Theme::color(available ? ui::VOLT : ui::MUTED));
    ActualCurrentValue.setColor(ui::Theme::color(available && currentValid ? ui::AMP : ui::MUTED));
    ActualVoltageValue.invalidate(); ActualCurrentValue.invalidate();
}

void Screen1View::setTemperaturesDeciC(int16_t mosDeciC, int16_t pcbDeciC)
{
    char text[16];
    psu_format_deci_c(text, sizeof(text), mosDeciC);
    lab_show(TemperatureValue,TemperatureValueBuffer,TEMPERATUREVALUE_SIZE,text,lab_text());
    psu_format_deci_c(text, sizeof(text), pcbDeciC);
    lab_show(PcbTemperatureValue,PcbTemperatureValueBuffer,PCBTEMPERATUREVALUE_SIZE,text,lab_text());
}

void Screen1View::syncControllerSetpoints(uint32_t mv, uint32_t ma)
{
    if (!replaceOnNextKey || (setVoltageMv == mv && currentLimitMa == ma)) return;
    setVoltageMv = mv; currentLimitMa = ma;
    refreshSetpoints(); loadEditorFromSetpoint();
}

void Screen1View::setHostAuxMetrics()
{
    G4Record rail, pd;
    g4_record_snapshot(psu_g4(),G4_RECORD_T,&rail);
    g4_record_snapshot(psu_g4(),G4_RECORD_TC,&pd);
    int64_t value;
    char text[20];
    if(rail.valid && psu_app_now()-rail.ms<=50U && g4_record_value(&rail,"vout_mv",&value))
        snprintf(text,sizeof(text),"%lu.%lu V",(unsigned long)(value/1000),(unsigned long)((value%1000)/100));
    else snprintf(text,sizeof(text),"-- V");
    lab_show(TemperatureValue,TemperatureValueBuffer,TEMPERATUREVALUE_SIZE,text,lab_text());
    if(pd.valid && psu_app_now()-pd.ms<=1000U && g4_record_value(&pd,"pd_mv",&value))
        snprintf(text,sizeof(text),"%lu.%lu V",(unsigned long)(value/1000),(unsigned long)((value%1000)/100));
    else snprintf(text,sizeof(text),"-- V");
    lab_show(PcbTemperatureValue,PcbTemperatureValueBuffer,PCBTEMPERATUREVALUE_SIZE,text,lab_text());
    TelemetryRecord battery(G4_RECORD_TB);
    int64_t mv=0,ma=0;char flow[32];
    const bool valid=battery.is("sample",1)&&battery.is("bms",1)&&battery.get("pack_mv",mv)&&battery.get("i_pack_ma",ma);
    if(valid){const int64_t mw=mv*ma/1000;snprintf(flow,sizeof(flow),"%s%ld.%01ld W",mw<0?"-":mw>0?"+":"",(long)(llabs(mw)/1000),(long)(llabs(mw)%1000/100));}
    else snprintf(flow,sizeof(flow),"-- W");
    lab_show(BatteryValue,BatteryValueBuffer,BATTERYVALUE_SIZE,flow,valid?(ma<0?lab_red():ma>0?lab_green():lab_text()):lab_muted());
    if(rail.valid && psu_app_now()-rail.ms<=50U && g4_record_value(&rail,"vout_mv",&mv))snprintf(flow,sizeof(flow),"%ld.%02ld V",(long)(mv/1000),(long)(mv%1000/10));
    else snprintf(flow,sizeof(flow),"-- V");
    lab_show(PowerValue,PowerValueBuffer,POWERVALUE_SIZE,flow,lab_text());

}

void Screen1View::saveSwipeEditor(MainSwipeEditorState& state) const
{
    state.target=static_cast<uint8_t>(editTarget); state.length=editLength;
    state.preset=selectedPreset; state.replace=replaceOnNextKey;
    memcpy(state.text,editAscii,sizeof(editAscii));
    memcpy(state.status,EditorStatusBuffer,sizeof(EditorStatusBuffer));
    state.statusColor=EditorStatus.getColor();
    state.statusTicks=editorNoticeTicks;
}
void Screen1View::restoreSwipeEditor(const MainSwipeEditorState& state)
{
    if(state.target==EDIT_CURRENT) selectCurrent(); else selectVoltage();
    editLength=state.length; replaceOnNextKey=state.replace;
    memcpy(editAscii,state.text,sizeof(editAscii));
    updatePresetHighlight(state.preset);
    refreshEditor();
    memcpy(EditorStatusBuffer,state.status,sizeof(EditorStatusBuffer));
    EditorStatus.setColor(state.statusColor);
    editorNoticeTicks=state.statusTicks;
    EditorStatus.setVisible(false);
    EditorStatus.invalidate();
}

void Screen1View::setupTheme()
{
    ui::ThemeScreen& theme=ui::ThemeScreen::get();
    theme.begin(*this);
    theme.box(Background,ui::BACKGROUND);
    theme.box(ThemeHeader,ui::SURFACE);
    theme.box(MainHeaderDivider,ui::BORDER);
    theme.panel(ThemeVoltageCard);
    theme.panel(ThemeCurrentCard);
    theme.panel(ThemeKeypadCard);
    theme.text(TitleText);
    theme.image(ModePill,ui::PILL);
    theme.text(ModeTextFront,ui::ON_ACCENT);
    theme.text(TemperatureLabel);
    theme.text(TemperatureValue);
    theme.text(PowerLabel);
    theme.text(PowerValue);
    theme.button(OutputEnable,ui::OUTPUT);
    theme.text(OutputLabel);
    theme.text(ActualVoltageLabel);
    theme.text(ActualVoltageValue,ui::VOLT);
    theme.text(SetVoltageLabel);
    theme.text(SetVoltageValue);
    theme.text(ActualCurrentLabel);
    theme.text(ActualCurrentValue,ui::AMP);
    theme.text(SetCurrentLabel);
    theme.text(SetCurrentValue);
    theme.button(Key1,ui::NORMAL);
    theme.button(Key2,ui::NORMAL);
    theme.button(Key3,ui::NORMAL);
    theme.button(Key4,ui::NORMAL);
    theme.button(Key5,ui::NORMAL);
    theme.button(Key6,ui::NORMAL);
    theme.button(Key7,ui::NORMAL);
    theme.button(Key8,ui::NORMAL);
    theme.button(Key9,ui::NORMAL);
    theme.button(KeyClear,ui::NORMAL);
    theme.button(Key0,ui::NORMAL);
    theme.button(KeyDot,ui::NORMAL);
    theme.button(KeyBack,ui::NORMAL);
    theme.button(KeyEnter,ui::PRIMARY);
    theme.button(Preset1Button,ui::NORMAL);
    theme.button(Preset2Button,ui::NORMAL);
    theme.button(Preset3Button,ui::NORMAL);
    theme.button(RemoteSenseButton,ui::NORMAL);
    theme.text(RemoteSenseText);
    theme.button(VoltMinus);theme.button(VoltStep,ui::PRIMARY);theme.button(VoltPlus);
    theme.button(AmpMinus);theme.button(AmpStep,ui::PRIMARY);theme.button(AmpPlus);
    theme.button(SettingsButton,ui::NORMAL);
    theme.button(AllOffButton,ui::DANGER);
    theme.text(BatteryLabel);
    theme.text(BatteryValue);
    theme.text(PcbTemperatureLabel);
    theme.text(PcbTemperatureValue);
    theme.image(VoltageSelection,ui::SELECTION);
    theme.image(CurrentSelection,ui::SELECTION);
    theme.text(VoltageMaxHint);
    theme.text(CurrentMaxHint);
    theme.text(LinkStatus);
    theme.text(EditorStatus);
    theme.text(EditorHelp);
    theme.apply();
}

void Screen1View::handleTickEvent()
{
    if(static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->isScreenTransitionActive())return;
    refreshRemoteSense();
    
    if (++stripDivider >= 6) { stripDivider = 0; refreshPowerStrip(); }
    syncKit();
    if(editorNoticeTicks && --editorNoticeTicks==0) {
        EditorStatus.setVisible(false);
        EditorStatus.invalidate();
    }
}

void Screen1View::allOff() { showEditorStatus(psu_app_power_shutdown()?"POWER OFF requested / wake TS2":"POWER OFF request failed",true); }

void Screen1View::refreshRemoteSense(){
 TelemetryRecord aux(G4_RECORD_AUX);int64_t flags=0,code=0;
 const bool fresh=aux.get("sense_flags",flags)&&aux.get("sense_code",code);
 const char* label=!fresh?"SENSE\nNO DATA":flags&4?"SENSE\nFAULT":flags&1?"SENSE\nREMOTE":flags&2?"SENSE\nARMED":"SENSE\nLOCAL";
 lab_show(RemoteSenseText,RemoteSenseTextBuffer,REMOTESENSETEXT_SIZE,label,
     !fresh?lab_muted():(flags&4)?lab_red():(flags&2 && !(flags&1))?lab_amber():lab_text());
 const touchgfx::BitmapId released=fresh&&(flags&1)?BITMAP_MAIN_SENSE_SEL_148X60_ID:BITMAP_MAIN_SENSE_REL_148X60_ID;
 if(RemoteSenseButton.getCurrentlyDisplayedBitmap().getId()!=released && !RemoteSenseButton.getPressedState()){
     RemoteSenseButton.setBitmaps(touchgfx::Bitmap(released),touchgfx::Bitmap(BITMAP_MAIN_SENSE_SEL_148X60_ID));RemoteSenseButton.invalidate();
 }
 lab_enable(RemoteSenseButton,fresh);
}
void Screen1View::toggleRemoteSense(){
 TelemetryRecord aux(G4_RECORD_AUX);int64_t flags=0;
 if(!aux.get("sense_flags",flags)){showEditorStatus("No fresh SENSE data",true);return;}
 if(flags&4){
     static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->gotoScreenPowerPathScreenWipeTransitionWest();return;
 }
 const bool accepted=g4_remote(psu_g4(),(flags&2)==0,0)!=0;
 showEditorStatus(accepted?"SENSE requested; waiting for telemetry":"SENSE request blocked",!accepted);
 refreshRemoteSense();
}

void Screen1View::nudgeVoltage(bool up) {
    const uint32_t steps[]={10,100,1000};
    const uint32_t step=steps[voltageStep];
    selectVoltage();
    setVoltageMv=up?(setVoltageMv<=27000U-step?setVoltageMv+step:27000U):(setVoltageMv>=step?setVoltageMv-step:0U);
    updatePresetHighlight(0);
    submitSetpoints();
}
void Screen1View::nudgeVoltUp(){nudgeVoltage(true);}
void Screen1View::nudgeVoltDown(){nudgeVoltage(false);}
void Screen1View::stepVolt(){
    voltageStep=(voltageStep+1)%3;
    const touchgfx::TypedTextId labels[]={T_TXT_TASK_STEP_0,T_TXT_TASK_STEP_1,T_TXT_TASK_STEP_2};
    VoltStep.setLabelText(touchgfx::TypedText(labels[voltageStep]));VoltStep.invalidate();
    const char* messages[]={"Voltage step: 10 mV","Voltage step: 100 mV","Voltage step: 1 V"};
    showEditorStatus(messages[voltageStep]);
}

void Screen1View::nudgeCurrent(bool up) {
    const uint32_t steps[]={10,100,1000};
    const uint32_t step=steps[currentStep];
    selectCurrent();
    currentLimitMa=up?(currentLimitMa<=5000U-step?currentLimitMa+step:5000U):(currentLimitMa>=step?currentLimitMa-step:0U);
    updatePresetHighlight(0);
    submitSetpoints();
}
void Screen1View::nudgeAmpUp(){nudgeCurrent(true);}
void Screen1View::nudgeAmpDown(){nudgeCurrent(false);}
void Screen1View::stepAmp(){
    currentStep=(currentStep+1)%3;
    const touchgfx::TypedTextId labels[]={T_TXT_TASK_STEP_0,T_TXT_TASK_STEP_1,T_TXT_TASK_STEP_2};
    AmpStep.setLabelText(touchgfx::TypedText(labels[currentStep]));AmpStep.invalidate();
    const char* messages[]={"Current step: 10 mA","Current step: 100 mA","Current step: 1 A"};
    showEditorStatus(messages[currentStep]);
}
