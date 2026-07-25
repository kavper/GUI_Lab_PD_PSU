#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/Color.hpp>
#include <images/BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef SIMULATOR
extern "C" void PSU_SetOutputLed(uint8_t enabled);
extern "C" void LDO_SetLimits(uint32_t voltage_mv, uint32_t current_ma);
extern "C" void LDO_SetOutput(uint8_t enabled);
#endif

Screen1View::Screen1View()
    : editTarget(EDIT_VOLTAGE),
      setVoltageMv(12000),
      currentLimitMa(2000),
      measuredCurrentMa(0),
      outputEnabled(false),
      constantCurrentMode(false),
      editLength(0),
      replaceOnNextKey(false),
      selectedPreset(0)
{
    editAscii[0] = '\0';
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
    const touchgfx::colortype light = touchgfx::Color::getColorFromRGB(238, 244, 251);
    const touchgfx::colortype dark = touchgfx::Color::getColorFromRGB(5, 19, 26);
    Key0.setLabelColor(light); Key1.setLabelColor(light); Key2.setLabelColor(light);
    Key3.setLabelColor(light); Key4.setLabelColor(light); Key5.setLabelColor(light);
    Key6.setLabelColor(light); Key7.setLabelColor(light); Key8.setLabelColor(light);
    Key9.setLabelColor(light); KeyDot.setLabelColor(light); KeyClear.setLabelColor(light);
    KeyBack.setLabelColor(light);
    Preset1Button.setLabelColor(light);
    Preset2Button.setLabelColor(light);
    Preset3Button.setLabelColor(light);
    KeyEnter.setLabelColor(dark);
    OutputLabel.setColor(dark);
    refreshSetpoints();
    selectVoltage();
#ifdef SIMULATOR
    setMeasurements(12040, 1236000, 386);
#else
    setMeasurements(0, 0, 0);
#endif
    setInputMetrics(20000, 14900);
    setPcbTemperature(412);
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    const bool overKeypad = event.getX() >= 510 && event.getX() < 800 &&
                            event.getY() >= 70 && event.getY() < 480;
    if (event.getType() != touchgfx::GestureEvent::SWIPE_VERTICAL || !overKeypad)
    {
        Screen1ViewBase::handleGestureEvent(event);
        return;
    }

    const bool increase = event.getVelocity() < 0;
    updatePresetHighlight(0);

    if (editTarget == EDIT_VOLTAGE)
    {
        const uint32_t stepMv = 100U;
        if (increase)
            setVoltageMv = setVoltageMv <= (27000U - stepMv)
                ? setVoltageMv + stepMv : 27000U;
        else
            setVoltageMv = setVoltageMv >= stepMv
                ? setVoltageMv - stepMv : 0U;
    }
    else
    {
        const uint32_t stepMa = 50U;
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
#ifndef SIMULATOR
    LDO_SetLimits(setVoltageMv, currentLimitMa);
#endif
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
    const int16_t absTemperature = temperatureDeciC < 0 ? -temperatureDeciC : temperatureDeciC;
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
    if (outputEnabled == enabled)
        return;
    outputEnabled = enabled;
    OutputEnable.forceState(enabled);
    touchgfx::Unicode::snprintf(OutputLabelBuffer, OUTPUTLABEL_SIZE,
                               enabled ? "OUTPUT ON" : "OUTPUT OFF");
    OutputLabel.setColor(enabled
        ? touchgfx::Color::getColorFromRGB(255, 255, 255)
        : touchgfx::Color::getColorFromRGB(5, 19, 26));
    OutputEnable.invalidate();
    OutputLabel.invalidate();
#ifndef SIMULATOR
    PSU_SetOutputLed(enabled ? 1U : 0U);
#endif
}

void Screen1View::setPcbTemperature(int16_t temperatureDeciC)
{
    const int16_t absolute = temperatureDeciC < 0 ? -temperatureDeciC : temperatureDeciC;
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
    if (setVoltageMv < 10000)
        touchgfx::Unicode::snprintf(SetVoltageValueBuffer, SETVOLTAGEVALUE_SIZE,
                                   "%u.%03u V", setVoltageMv / 1000, setVoltageMv % 1000);
    else
        touchgfx::Unicode::snprintf(SetVoltageValueBuffer, SETVOLTAGEVALUE_SIZE,
                                   "%u.%02u V", setVoltageMv / 1000, (setVoltageMv % 1000) / 10);
    touchgfx::Unicode::snprintf(SetCurrentValueBuffer, SETCURRENTVALUE_SIZE,
                               "%u.%03u A", currentLimitMa / 1000, currentLimitMa % 1000);
    SetVoltageValue.invalidate();
    SetCurrentValue.invalidate();
}

void Screen1View::loadEditorFromSetpoint()
{
    const uint32_t value = (editTarget == EDIT_VOLTAGE) ? setVoltageMv : currentLimitMa;
    const uint32_t decimals = (editTarget == EDIT_VOLTAGE && value >= 10000) ? 2 : 3;
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

void Screen1View::refreshEditor()
{
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
    editTarget = EDIT_VOLTAGE;
    VoltageSelection.setVisible(true);
    CurrentSelection.setVisible(false);
    VoltageSelection.invalidate();
    CurrentSelection.invalidate();
    SetVoltageLabel.setColor(touchgfx::Color::getColorFromRGB(42, 199, 217));
    SetVoltageValue.setColor(touchgfx::Color::getColorFromRGB(42, 199, 217));
    SetCurrentLabel.setColor(touchgfx::Color::getColorFromRGB(147, 163, 184));
    SetCurrentValue.setColor(touchgfx::Color::getColorFromRGB(238, 244, 251));
    SetVoltageLabel.invalidate(); SetVoltageValue.invalidate();
    SetCurrentLabel.invalidate(); SetCurrentValue.invalidate();
    loadEditorFromSetpoint();
    replaceOnNextKey = true;
}

void Screen1View::selectCurrent()
{
    editTarget = EDIT_CURRENT;
    VoltageSelection.setVisible(false);
    CurrentSelection.setVisible(true);
    VoltageSelection.invalidate();
    CurrentSelection.invalidate();
    SetVoltageLabel.setColor(touchgfx::Color::getColorFromRGB(147, 163, 184));
    SetVoltageValue.setColor(touchgfx::Color::getColorFromRGB(238, 244, 251));
    SetCurrentLabel.setColor(touchgfx::Color::getColorFromRGB(42, 199, 217));
    SetCurrentValue.setColor(touchgfx::Color::getColorFromRGB(42, 199, 217));
    SetVoltageLabel.invalidate(); SetVoltageValue.invalidate();
    SetCurrentLabel.invalidate(); SetCurrentValue.invalidate();
    loadEditorFromSetpoint();
    replaceOnNextKey = true;
}

void Screen1View::outputToggled()
{
    outputEnabled = OutputEnable.getState();
    touchgfx::Unicode::snprintf(OutputLabelBuffer, OUTPUTLABEL_SIZE,
                               outputEnabled ? "OUTPUT ON" : "OUTPUT OFF");
    OutputLabel.setColor(outputEnabled
        ? touchgfx::Color::getColorFromRGB(255, 255, 255)
        : touchgfx::Color::getColorFromRGB(5, 19, 26));
    OutputLabel.invalidate();
    setRegulationMode(outputEnabled && measuredCurrentMa >= (int32_t)currentLimitMa);
#ifndef SIMULATOR
    PSU_SetOutputLed(outputEnabled ? 1U : 0U);
    if (outputEnabled)
        LDO_SetLimits(setVoltageMv, currentLimitMa);
    LDO_SetOutput(outputEnabled ? 1U : 0U);
#endif
    // The Presenter/Model can forward this state to the hardware driver.
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
    if (replaceOnNextKey)
    {
        replaceOnNextKey = false;
        editLength = 0;
        editAscii[0] = '\0';
        refreshEditor();
        return;
    }
    if (editLength > 0)
    {
        editAscii[--editLength] = '\0';
        refreshEditor();
    }
}

void Screen1View::keyEnter()
{
    updatePresetHighlight(0);
    const double entered = strtod(editAscii, 0);
    if (editTarget == EDIT_VOLTAGE)
    {
        const double limited = entered < 0.0 ? 0.0 : (entered > 27.0 ? 27.0 : entered);
        setVoltageMv = static_cast<uint32_t>(limited * 1000.0 + 0.5);
    }
    else
    {
        // Demonstration range: 0 to 5 A.
        const double limited = entered < 0.0 ? 0.0 : (entered > 5.0 ? 5.0 : entered);
        currentLimitMa = static_cast<uint32_t>(limited * 1000.0 + 0.5);
    }
    refreshSetpoints();
    loadEditorFromSetpoint();
    replaceOnNextKey = true;
    setRegulationMode(outputEnabled && measuredCurrentMa >= (int32_t)currentLimitMa);
#ifndef SIMULATOR
    LDO_SetLimits(setVoltageMv, currentLimitMa);
#endif
}

void Screen1View::updatePresetHighlight(uint8_t preset)
{
    selectedPreset = preset;

    const touchgfx::Bitmap released(BITMAP_BTN_PRESET_V3_RELEASED_140X50_ID);
    const touchgfx::Bitmap selected(BITMAP_BTN_PRESET_V3_PRESSED_140X50_ID);
    const touchgfx::colortype normalText = touchgfx::Color::getColorFromRGB(238, 244, 251);
    const touchgfx::colortype selectedText = touchgfx::Color::getColorFromRGB(42, 199, 217);

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
#ifndef SIMULATOR
    LDO_SetLimits(setVoltageMv, currentLimitMa);
#endif
}

void Screen1View::preset1() { applyPreset(1, 5000, 1000); }
void Screen1View::preset2() { applyPreset(2, 12000, 2000); }
void Screen1View::preset3() { applyPreset(3, 20000, 3000); }
