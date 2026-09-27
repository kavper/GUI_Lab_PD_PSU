#include <gui/screenprotection_screen/ScreenProtectionView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenProtectionView::ScreenProtectionView()
    : divider(0)
{
}

void ScreenProtectionView::setupScreen()
{
    ScreenProtectionViewBase::setupScreen();
    refresh();
}

void ScreenProtectionView::tearDownScreen()
{
    ScreenProtectionViewBase::tearDownScreen();
}

void ScreenProtectionView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenProtectionView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_protection(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), ProtectBodyBuffer, PROTECTBODY_SIZE);
    ProtectBody.invalidate();
}

void ScreenProtectionView::clearFault()
{
    psu_app_clear_fault();
    refresh();
}

