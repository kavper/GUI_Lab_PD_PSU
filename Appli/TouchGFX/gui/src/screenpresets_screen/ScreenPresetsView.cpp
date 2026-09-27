#include <gui/screenpresets_screen/ScreenPresetsView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenPresetsView::ScreenPresetsView()
    : divider(0)
{
}

void ScreenPresetsView::setupScreen()
{
    ScreenPresetsViewBase::setupScreen();
    refresh();
}

void ScreenPresetsView::tearDownScreen()
{
    ScreenPresetsViewBase::tearDownScreen();
}

void ScreenPresetsView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenPresetsView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_presets(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), PresetBodyBuffer, PRESETBODY_SIZE);
    PresetBody.invalidate();
}

void ScreenPresetsView::loadPreset()
{
    { static uint8_t slot; psu_preset_apply(slot); slot = (uint8_t)((slot + 1U) % 3U); }
    refresh();
}
void ScreenPresetsView::savePreset()
{
    psu_preset_save_current(0, "USER");
    refresh();
}
void ScreenPresetsView::duplicatePreset()
{
    psu_preset_duplicate(0, 3);
    refresh();
}
void ScreenPresetsView::resetPresets()
{
    psu_preset_reset_defaults();
    refresh();
}

