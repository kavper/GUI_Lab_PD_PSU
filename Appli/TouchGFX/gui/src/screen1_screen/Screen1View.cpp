#include <gui/common/UiTheme.hpp>
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
      editorNoticeTicks(0)
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

    setupTheme();
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

void Screen1View::setMeasurements(uint32_t voltageMv, int32_t currentUa, int16_t temperatureDeciC)
{
    if (voltageMv < 10000)
        touchgfx::Unicode::snprintf(ActualVoltageValueBuffer, ACTUALVOLTAGEVALUE_SIZE,
                                   "%u.%03u V", voltageMv / 1000, voltageMv % 1000);
    else
        touchgfx::Unicode::snprintf(ActualVoltageValueBuffer, ACTUALVOLTAGEVALUE_SIZE,
                                   "%u.%02u V", voltageMv / 1000, (voltageMv % 1000) / 10);
    const int32_t displayCurrentUa = currentUa > 0 ? currentUa : 0;
    const int32_t roundedCurrentMa = (displayCurrentUa + 500) / 1000;
    touchgfx::Unicode::snprintf(ActualCurrentValueBuffer, ACTUALCURRENTVALUE_SIZE,
                               "%d.%03d A", (int)(roundedCurrentMa / 1000),
                               (int)(roundedCurrentMa % 1000));
    const int32_t absTemperature = temperatureDeciC < 0 ? -(int32_t)temperatureDeciC : (int32_t)temperatureDeciC;
    if (temperatureDeciC < 0)
        touchgfx::Unicode::snprintf(TemperatureValueBuffer, TEMPERATUREVALUE_SIZE,
                                   "-%d.%d C", absTemperature / 10, absTemperature % 10);
    else
        touchgfx::Unicode::snprintf(TemperatureValueBuffer, TEMPERATUREVALUE_SIZE,
                                   "%d.%d C", absTemperature / 10, absTemperature % 10);
    ActualVoltageValue.invalidate();
    ActualCurrentValue.invalidate();
    TemperatureValue.invalidate();
    measuredCurrentMa = displayCurrentUa / 1000;
    setRegulationMode(outputEnabled && measuredCurrentMa >= (int32_t)currentLimitMa);
}

void Screen1View::setInputMetrics(uint32_t inputVoltageMv, uint32_t outputPowerMw)
{
    if (inputVoltageMv < 10000U)
        touchgfx::Unicode::snprintf(BatteryValueBuffer, BATTERYVALUE_SIZE,
                                   "%u.%03u V", inputVoltageMv / 1000U,
                                   inputVoltageMv % 1000U);
    else
        touchgfx::Unicode::snprintf(BatteryValueBuffer, BATTERYVALUE_SIZE,
                                   "%u.%02u V", inputVoltageMv / 1000U,
                                   (inputVoltageMv % 1000U) / 10U);
    touchgfx::Unicode::snprintf(PowerValueBuffer, POWERVALUE_SIZE,
                               "%u.%02u W", outputPowerMw / 1000U,
                               (outputPowerMw % 1000U) / 10U);
    BatteryValue.invalidate();
    PowerValue.invalidate();
}

void Screen1View::setCurrentMeasurementCalibrated(bool calibrated)
{
    ActualCurrentLabel.setTypedText(touchgfx::TypedText(calibrated
        ? T_TXT_ACTUAL_I
        : T_TXT_ACTUAL_I_UNCAL));
    ActualCurrentLabel.invalidate();
}

void Screen1View::setControllerOutputState(bool enabled)
{
    PsuSnapshot live;
    psu_snapshot(&live);
    outputEnabled = enabled;
    OutputEnable.forceState(enabled);
    // The control label describes the action that a press will perform.  The
    // previous "PSU OFF" label described the current state and made the
    // toggle's behaviour ambiguous.
    const char *label = live.shutdown_pending ? "TURNING OFF..." : !live.g0_connected ? "NO LINK" : live.output_requested != enabled
        ? (live.output_requested ? "TURNING ON..." : "TURNING OFF...")
        : (enabled ? "TURN OFF" : "TURN ON");
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(label), OutputLabelBuffer, OUTPUTLABEL_SIZE);
    OutputLabel.setColor(enabled ? touchgfx::Color::getColorFromRGB(255,255,255) : touchgfx::Color::getColorFromRGB(23,35,55));
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
    EditorStatus.setVisible(true);
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
    VoltageSelection.setVisible(true);
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
    CurrentSelection.setVisible(true);
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
    else psu_app_set_output(1, PSU_SRC_LCD);
    psu_snapshot(&live);
    setControllerOutputState(live.output_confirmed != 0);
}

void Screen1View::setRegulationMode(bool constantCurrent)
{
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
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(text ? text : "G4 OFFLINE"),
                               LinkStatusBuffer, LINKSTATUS_SIZE);
    LinkStatus.setColor(touchgfx::Color::getColorFromRGB(
        text && strstr(text, "ONLINE") ? 22 : 168,
        text && strstr(text, "ONLINE") ? 117 : 91,
        text && strstr(text, "ONLINE") ? 72 : 5));
    LinkStatus.invalidate();
}

void Screen1View::updatePresetHighlight(uint8_t preset)
{
    selectedPreset = preset;
    {const PsuPreset* p=psu_preset_get(0);char b[40];if(p){snprintf(b,sizeof(b),"P1  %lu.%02lu V\n%lu.%03lu A",(unsigned long)(p->voltage_mv/1000),(unsigned long)(p->voltage_mv%1000/10),(unsigned long)(p->current_ma/1000),(unsigned long)(p->current_ma%1000));touchgfx::Unicode::fromUTF8((const uint8_t*)b,QuickPreset1Buffer,QUICKPRESET1_SIZE);QuickPreset1.invalidate();}}
    {const PsuPreset* p=psu_preset_get(1);char b[40];if(p){snprintf(b,sizeof(b),"P2  %lu.%02lu V\n%lu.%03lu A",(unsigned long)(p->voltage_mv/1000),(unsigned long)(p->voltage_mv%1000/10),(unsigned long)(p->current_ma/1000),(unsigned long)(p->current_ma%1000));touchgfx::Unicode::fromUTF8((const uint8_t*)b,QuickPreset2Buffer,QUICKPRESET2_SIZE);QuickPreset2.invalidate();}}
    {const PsuPreset* p=psu_preset_get(2);char b[40];if(p){snprintf(b,sizeof(b),"P3  %lu.%02lu V\n%lu.%03lu A",(unsigned long)(p->voltage_mv/1000),(unsigned long)(p->voltage_mv%1000/10),(unsigned long)(p->current_ma/1000),(unsigned long)(p->current_ma%1000));touchgfx::Unicode::fromUTF8((const uint8_t*)b,QuickPreset3Buffer,QUICKPRESET3_SIZE);QuickPreset3.invalidate();}}


    const touchgfx::Bitmap released(BITMAP_MAIN_PRESET_REL_154X60_ID);
    const touchgfx::Bitmap selected(BITMAP_MAIN_PRESET_SEL_154X60_ID);
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
    showEditorStatus(!accepted ? "Blocked - check protection" :
                     limited ? "Limited by protection" : "Setpoint requested", !accepted || limited);
}

void Screen1View::setTelemetryAvailable(bool available, bool currentValid, bool temperatureValid)
{
    PsuSnapshot live;
    psu_snapshot(&live);
    const bool showMode = available && live.regulation_mode != 0;
    if (ModeTextFront.isVisible() != showMode) {
        ModeTextFront.setVisible(showMode); ModePill.setVisible(showMode);
        ModeTextFront.invalidate(); ModePill.invalidate();
    }
    if (!available) {
        touchgfx::Unicode::snprintf(ActualVoltageValueBuffer, ACTUALVOLTAGEVALUE_SIZE, "-- V");
        touchgfx::Unicode::snprintf(TemperatureValueBuffer, TEMPERATUREVALUE_SIZE, "-- C");
        touchgfx::Unicode::snprintf(PcbTemperatureValueBuffer, PCBTEMPERATUREVALUE_SIZE, "-- C");
        touchgfx::Unicode::snprintf(BatteryValueBuffer, BATTERYVALUE_SIZE, "-- V");
        TemperatureValue.invalidate(); PcbTemperatureValue.invalidate(); BatteryValue.invalidate();
    }
    if (!temperatureValid) {
        touchgfx::Unicode::snprintf(TemperatureValueBuffer, TEMPERATUREVALUE_SIZE, "-- C");
        touchgfx::Unicode::snprintf(PcbTemperatureValueBuffer, PCBTEMPERATUREVALUE_SIZE, "-- C");
        TemperatureValue.invalidate(); PcbTemperatureValue.invalidate();
    }
    if (!available || !currentValid) {
        touchgfx::Unicode::snprintf(ActualCurrentValueBuffer, ACTUALCURRENTVALUE_SIZE, "-- A");
        touchgfx::Unicode::snprintf(PowerValueBuffer, POWERVALUE_SIZE, "-- W");
        PowerValue.invalidate();
    }
    ActualVoltageValue.setColor(available ? touchgfx::Color::getColorFromRGB(23, 35, 55) : touchgfx::Color::getColorFromRGB(96, 112, 133));
    ActualCurrentValue.setColor(available && currentValid ? touchgfx::Color::getColorFromRGB(23, 35, 55) : touchgfx::Color::getColorFromRGB(96, 112, 133));
    ActualVoltageValue.invalidate(); ActualCurrentValue.invalidate();
}

void Screen1View::setTemperaturesDeciC(int16_t mosDeciC, int16_t pcbDeciC)
{
    char text[16];
    psu_format_deci_c(text, sizeof(text), mosDeciC);
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(text), TemperatureValueBuffer, TEMPERATUREVALUE_SIZE);
    psu_format_deci_c(text, sizeof(text), pcbDeciC);
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(text), PcbTemperatureValueBuffer, PCBTEMPERATUREVALUE_SIZE);
    TemperatureValue.invalidate(); PcbTemperatureValue.invalidate();
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
    if(rail.valid && psu_app_now()-rail.ms<=1500U && g4_record_value(&rail,"vout_mv",&value))
        snprintf(text,sizeof(text),"%lu.%lu V",(unsigned long)(value/1000),(unsigned long)((value%1000)/100));
    else snprintf(text,sizeof(text),"-- V");
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(text),TemperatureValueBuffer,TEMPERATUREVALUE_SIZE);
    if(pd.valid && psu_app_now()-pd.ms<=1500U && g4_record_value(&pd,"pd_mv",&value))
        snprintf(text,sizeof(text),"%lu.%lu V",(unsigned long)(value/1000),(unsigned long)((value%1000)/100));
    else snprintf(text,sizeof(text),"-- V");
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(text),PcbTemperatureValueBuffer,PCBTEMPERATUREVALUE_SIZE);
    TemperatureValue.invalidate();PcbTemperatureValue.invalidate();
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
    EditorStatus.setVisible(editorNoticeTicks!=0);
    EditorStatus.invalidate();
}

void Screen1View::setupTheme()
{
    ui::ThemeScreen& theme=ui::ThemeScreen::get();
    theme.begin(*this);
    theme.box(Background,ui::BACKGROUND);
    theme.box(ThemeHeader,ui::SURFACE);
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
    theme.text(ActualVoltageValue);
    theme.text(SetVoltageLabel);
    theme.text(SetVoltageValue);
    theme.text(ActualCurrentLabel);
    theme.text(ActualCurrentValue);
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
    theme.button(SettingsButton,ui::NORMAL);
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
    theme.text(QuickPreset1);
    theme.text(QuickPreset2);
    theme.text(QuickPreset3);
    theme.apply();
}

void Screen1View::handleTickEvent()
{
    if(static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->isScreenTransitionActive())return;
    if(editorNoticeTicks && --editorNoticeTicks==0) {
        EditorStatus.setVisible(false);
        EditorStatus.invalidate();
    }
}
