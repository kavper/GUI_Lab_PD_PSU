#include <gui/screensequencer_screen/ScreenSequencerView.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" {
#include "psu_app.h"
}

ScreenSequencerView::ScreenSequencerView()
    : divider(0)
{
}

void ScreenSequencerView::setupScreen()
{
    ScreenSequencerViewBase::setupScreen();
    refresh();
}

void ScreenSequencerView::tearDownScreen()
{
    ScreenSequencerViewBase::tearDownScreen();
}

void ScreenSequencerView::handleTickEvent()
{
    if (++divider < 8)
        return;
    divider = 0;
    refresh();
}

void ScreenSequencerView::refresh()
{
    char ascii[800];
    psu_app_ensure();
    psu_render_sequencer(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii), SeqBodyBuffer, SEQBODY_SIZE);
    SeqBody.invalidate();
}

void ScreenSequencerView::seqRun()
{
    psu_seq_start(psu_sequencer(), psu_app_now());
    refresh();
}
void ScreenSequencerView::seqPause()
{
    psu_seq_pause(psu_sequencer(), psu_app_now());
    refresh();
}
void ScreenSequencerView::seqStop()
{
    psu_seq_stop(psu_sequencer(), psu_app_now(), 0);
    refresh();
}
void ScreenSequencerView::seqAdd()
{
    psu_seq_add(psu_sequencer());
    refresh();
}
void ScreenSequencerView::seqRemove()
{
    psu_seq_remove_selected(psu_sequencer());
    refresh();
}
void ScreenSequencerView::seqPrev()
{
    if (psu_sequencer()->selected > 0) psu_sequencer()->selected--;
    refresh();
}
void ScreenSequencerView::seqNext()
{
    if (psu_sequencer()->selected + 1 < psu_sequencer()->count) psu_sequencer()->selected++;
    refresh();
}

