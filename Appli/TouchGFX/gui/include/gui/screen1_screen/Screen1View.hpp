#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <touchgfx/events/GestureEvent.hpp>
#include <stdint.h>

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);

    // API dla warstwy sprzętowej/modelu. Jednostki: mV, mA i 0.1 °C.
    void setMeasurements(uint32_t voltageMv, int32_t currentUa, int16_t temperatureDeciC);
    void setInputMetrics(uint32_t inputVoltageMv, uint32_t outputPowerMw);
    void setPcbTemperature(int16_t temperatureDeciC);
    void setRegulationMode(bool constantCurrent);
    void setCurrentMeasurementCalibrated(bool calibrated);
    void setControllerOutputState(bool enabled);
    void setLinkStatus(const char *text);
    uint32_t getSetVoltageMv() const { return setVoltageMv; }
    uint32_t getCurrentLimitMa() const { return currentLimitMa; }
    bool isOutputEnabled() const { return outputEnabled; }

    virtual void outputToggled();
    virtual void selectVoltage();
    virtual void selectCurrent();
    virtual void key0(); virtual void key1(); virtual void key2(); virtual void key3();
    virtual void key4(); virtual void key5(); virtual void key6(); virtual void key7();
    virtual void key8(); virtual void key9(); virtual void keyDot();
    virtual void keyClear(); virtual void keyBack(); virtual void keyEnter();
    virtual void preset1(); virtual void preset2(); virtual void preset3();

protected:
    enum EditTarget { EDIT_VOLTAGE, EDIT_CURRENT };
    EditTarget editTarget;
    uint32_t setVoltageMv;
    uint32_t currentLimitMa;
    int32_t measuredCurrentMa;
    bool outputEnabled;
    bool constantCurrentMode;
    char editAscii[12];
    uint8_t editLength;
    bool replaceOnNextKey;
    uint8_t selectedPreset;

    void appendKey(char key);
    void loadEditorFromSetpoint();
    void refreshEditor();
    void refreshSetpoints();
    void applyPreset(uint8_t preset, uint32_t voltageMv, uint32_t currentMa);
    void updatePresetHighlight(uint8_t preset);
};

#endif // SCREEN1VIEW_HPP
