#include <gui/screenpresets_screen/ScreenPresetsView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_format.h"
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

namespace
{
template <typename Name, typename Volt, typename Amp>
void show_preset(uint8_t index, Name& name, touchgfx::Unicode::UnicodeChar* name_b, uint16_t name_n,
                 Volt& volt, touchgfx::Unicode::UnicodeChar* volt_b, uint16_t volt_n,
                 Amp& amp, touchgfx::Unicode::UnicodeChar* amp_b, uint16_t amp_n)
{
    const PsuPreset* preset = psu_preset_get(index);
    char label[17];
    char v[16];
    char a[16];
    unsigned i;
    if (preset == 0)
    {
        lab_show(name, name_b, name_n, "--", lab_muted());
        lab_show(volt, volt_b, volt_n, "--", lab_muted());
        lab_show(amp, amp_b, amp_n, "--", lab_muted());
        return;
    }
    for (i = 0; i < 16U; ++i)
        label[i] = preset->name[i];
    label[16] = '\0';
    if (label[0] == '\0')
        (void)snprintf(label, sizeof(label), "P%u", (unsigned)(index + 1U));
    psu_format_voltage(v, sizeof(v), preset->voltage_mv);
    psu_format_current_ma(a, sizeof(a), preset->current_ma);
    lab_show(name, name_b, name_n, label, lab_text());
    lab_show(volt, volt_b, volt_n, v, lab_cyan());
    lab_show(amp, amp_b, amp_n, a, lab_muted());
}
}

void ScreenPresetsView::refresh()
{
    psu_app_ensure();
    show_preset(0, P1Name, P1NameBuffer, P1NAME_SIZE, P1Volt, P1VoltBuffer, P1VOLT_SIZE, P1Amp, P1AmpBuffer, P1AMP_SIZE);
    show_preset(1, P2Name, P2NameBuffer, P2NAME_SIZE, P2Volt, P2VoltBuffer, P2VOLT_SIZE, P2Amp, P2AmpBuffer, P2AMP_SIZE);
    show_preset(2, P3Name, P3NameBuffer, P3NAME_SIZE, P3Volt, P3VoltBuffer, P3VOLT_SIZE, P3Amp, P3AmpBuffer, P3AMP_SIZE);
    show_preset(3, P4Name, P4NameBuffer, P4NAME_SIZE, P4Volt, P4VoltBuffer, P4VOLT_SIZE, P4Amp, P4AmpBuffer, P4AMP_SIZE);
    show_preset(4, P5Name, P5NameBuffer, P5NAME_SIZE, P5Volt, P5VoltBuffer, P5VOLT_SIZE, P5Amp, P5AmpBuffer, P5AMP_SIZE);
    show_preset(5, P6Name, P6NameBuffer, P6NAME_SIZE, P6Volt, P6VoltBuffer, P6VOLT_SIZE, P6Amp, P6AmpBuffer, P6AMP_SIZE);
    show_preset(6, P7Name, P7NameBuffer, P7NAME_SIZE, P7Volt, P7VoltBuffer, P7VOLT_SIZE, P7Amp, P7AmpBuffer, P7AMP_SIZE);
    show_preset(7, P8Name, P8NameBuffer, P8NAME_SIZE, P8Volt, P8VoltBuffer, P8VOLT_SIZE, P8Amp, P8AmpBuffer, P8AMP_SIZE);
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
