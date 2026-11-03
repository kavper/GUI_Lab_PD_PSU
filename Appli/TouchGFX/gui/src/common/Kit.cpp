#include <gui/common/Kit.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/TelemetryData.hpp>
#include <gui/common/TelemetryLayout.hpp>
#include <gui/common/TelemetryLayout.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/TypedText.hpp>
#include <touchgfx/Font.hpp>
#include <string.h>
#include <stdio.h>

namespace kit {

namespace {
Rect noSwipe[8];
unsigned noSwipeCount = 0;

void fill(const Rect& r, const Rect& clip, colortype c, uint8_t a = 255) { ui::fillRect(r, clip, c, a); }

touchgfx::colortype mix(colortype a, colortype b, unsigned weightB /*0..64*/)
{
    using touchgfx::Color;
    const unsigned wa = 64 - weightB;
    return Color::getColorFromRGB((Color::getRed(a) * wa + Color::getRed(b) * weightB) / 64,
                                  (Color::getGreen(a) * wa + Color::getGreen(b) * weightB) / 64,
                                  (Color::getBlue(a) * wa + Color::getBlue(b) * weightB) / 64);
}

void put(touchgfx::Unicode::UnicodeChar* dst, uint16_t n, const char* utf8)
{
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(utf8 ? utf8 : ""), dst, n);
}

bool same(const touchgfx::Unicode::UnicodeChar* buf, uint16_t n, const char* ascii)
{
    if (!ascii) ascii = "";
    for (uint16_t i = 0; i < n; ++i) {
        const uint8_t c = static_cast<uint8_t>(ascii[i]);
        if (c >= 0x80 || buf[i] != c) return false;
        if (!c) return true;
    }
    return false;
}

// Draw a text buffer inside box with a kit typography, vertically centred.
// Live values can be longer than the layout expects: the text then steps down
// to the next smaller size of the same family instead of being cut off.
void drawText(const touchgfx::Unicode::UnicodeChar* text, Font font, Align align, const Rect& box, const Rect& clip, colortype ink, int dy = 0)
{
    if (!text || !text[0]) return;
    font = fitFont(text, font, box.width);
    touchgfx::TextAreaWithOneWildcard t;
    t.setTypedText(touchgfx::TypedText(textId(font, align)));
    t.setWildcard(const_cast<touchgfx::Unicode::UnicodeChar*>(text));
    t.setColor(ink);
    const touchgfx::Font* f = t.getTypedText().getFont();
    const int h = f->getHeight();
    const int top = (box.height - h) / 2 + dy;
    t.setPosition(box.x, box.y + top, box.width, h + 4);
    Rect local = clip;
    local.x -= box.x;
    local.y -= box.y + top;
    local &= Rect(0, 0, box.width, h + 4);
    if (!local.isEmpty()) t.draw(local);
}

int textWidth(const touchgfx::Unicode::UnicodeChar* text, Font font)
{
    const touchgfx::Font* f = touchgfx::TypedText(textId(font, LEFT)).getFont();
    return f ? f->getStringWidth(text) : 0;
}
}

touchgfx::TypedTextId textId(Font font, Align align)
{
    static const touchgfx::TypedTextId ids[][3] = {
        { T_K_KCAP_L, T_K_KCAP_C, T_K_KCAP_R },
        { T_K_KSUB_L, T_K_KSUB_C, T_K_KSUB_R },
        { T_K_KBODY_L, T_K_KBODY_C, T_K_KBODY_R },
        { T_K_KBODYM_L, T_K_KBODYM_C, T_K_KBODYM_R },
        { T_K_KLABEL_L, T_K_KLABEL_C, T_K_KLABEL_R },
        { T_K_KTITLE_L, T_K_KTITLE_C, T_K_KTITLE_R },
        { T_K_KHEAD_L, T_K_KHEAD_C, T_K_KHEAD_R },
        { T_K_KM12_L, T_K_KM12_C, T_K_KM12_R },
        { T_K_KM14_L, T_K_KM14_C, T_K_KM14_R },
        { T_K_KM16_L, T_K_KM16_C, T_K_KM16_R },
        { T_K_KM20_L, T_K_KM20_C, T_K_KM20_R },
        { T_K_KM26_L, T_K_KM26_C, T_K_KM26_R },
        { T_K_KM34_L, T_K_KM34_C, T_K_KM34_R },
        { T_K_KM52_L, T_K_KM52_C, T_K_KM52_R },
        { T_K_KM60_L, T_K_KM60_C, T_K_KM60_R },
    };
    return ids[font][align];
}

void retype(touchgfx::TextArea& text, Font font, Align align)
{
    text.invalidate();
    text.setTypedText(touchgfx::TypedText(textId(font, align)));
    text.invalidate();
}

void hide(touchgfx::Drawable& d) { if (d.isVisible()) { d.invalidate(); d.setVisible(false); } }
void place(touchgfx::Drawable& d, int x, int y, int w, int h) { d.invalidate(); d.setPosition(x, y, w, h); d.invalidate(); }

void hideAll(touchgfx::Container& root)
{
    for (touchgfx::Drawable* d = root.getFirstChild(); d; d = d->getNextSibling()) d->setVisible(false);
    root.invalidate();
}

void clearNoSwipe() { noSwipeCount = 0; }
void addNoSwipe(const Rect& r) { if (noSwipeCount < 8) noSwipe[noSwipeCount++] = r; }
bool swipeBlocked(int x, int y)
{
    for (unsigned i = 0; i < noSwipeCount; ++i) if (noSwipe[i].intersect(x, y)) return true;
    return false;
}

// ---------------------------------------------------------------- fitting
void (*overflowHook)(const touchgfx::Unicode::UnicodeChar* text, int need, int have, bool fitted) = 0;

Font smaller(Font f)
{
    switch (f) {
    case HEAD: return TITLE; case TITLE: return LABEL; case LABEL: return BODYM;
    case BODYM: return BODY; case BODY: return SUB; case SUB: return CAP;
    case M60: return M52; case M52: return M34; case M34: return M26; case M26: return M20;
    case M20: return M16; case M16: return M14; case M14: return M12;
    default: return f;
    }
}

Font fitFont(const touchgfx::Unicode::UnicodeChar* text, Font font, int width)
{
    int need = textWidth(text, font);
    if (need <= width) return font;
    const int first = need;
    Font f = font;
    while (need > width && smaller(f) != f) { f = smaller(f); need = textWidth(text, f); }
    if (overflowHook) overflowHook(text, first, width, need <= width);
    return f;
}

// ---------------------------------------------------------------- Label
Label::Label() : ink(ui::TEXT), kfont(BODY), kalign(LEFT)
{
    buffer[0] = 0;
    setWildcard(buffer);
}
void Label::setup(Font font, Align align, int x, int y, int w, int h, ui::Role r)
{
    kfont = font; kalign = align;
    setTypedText(touchgfx::TypedText(textId(font, align)));
    setPosition(x, y, w, h);
    ink = r;
    setColor(col(r));
}
void Label::set(const char* utf8)
{
    if (same(buffer, SIZE, utf8)) return;
    invalidate();
    put(buffer, SIZE, utf8);
    invalidate();
}
void Label::setUnicode(const touchgfx::Unicode::UnicodeChar* t)
{
    invalidate();
    touchgfx::Unicode::strncpy(buffer, t, SIZE - 1); buffer[SIZE - 1] = 0;
    invalidate();
}
void Label::set(const char* utf8, ui::Role r) { setRole(r); set(utf8); }
void Label::setRole(ui::Role r) { if (r != ink) { ink = r; invalidate(); } }
void Label::draw(const Rect& area) const
{
    Label* self = const_cast<Label*>(this);
    self->setColor(col(ink));
    // Multi-line labels are sized by their author; single lines shrink to fit.
    bool multi = false;
    for (const touchgfx::Unicode::UnicodeChar* c = buffer; *c; ++c) if (*c == '\n') { multi = true; break; }
    const Font fitted = multi ? kfont : fitFont(buffer, kfont, getWidth());
    if (fitted == kfont) { touchgfx::TextAreaWithOneWildcard::draw(area); return; }
    const touchgfx::TypedText original = getTypedText();
    self->setTypedText(touchgfx::TypedText(textId(fitted, kalign)));
    // Keep the shrunk line on the same vertical centre.
    const int shift = (touchgfx::TypedText(textId(kfont, kalign)).getFont()->getHeight() - touchgfx::TypedText(textId(fitted, kalign)).getFont()->getHeight()) / 2;
    if (shift > 0) {
        Rect a = area; a.y -= shift;
        self->moveRelative(0, shift);
        touchgfx::TextAreaWithOneWildcard::draw(a);
        self->moveRelative(0, -shift);
    } else touchgfx::TextAreaWithOneWildcard::draw(area);
    self->setTypedText(original);
}

// ---------------------------------------------------------------- surfaces
void Panel::setup(int x, int y, int w, int h, ui::Role f, ui::Role b, ui::Role base, int r)
{
    setPosition(x, y, w, h);
    fillRole = f; borderRole = b; baseRole = base; radius = r;
}
void Panel::setBorder(ui::Role b) { if (b != borderRole) { borderRole = b; invalidate(); } }
void Panel::setFill(ui::Role f) { if (f != fillRole) { fillRole = f; invalidate(); } }
void Panel::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    ui::drawRounded(r, clip, col(borderRole), col(fillRole), col(baseRole), radius);
}

void Fill::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    fill(r, clip, col(role), alpha);
}

void Bar::set(int permille, ui::Role fillRole)
{
    if (permille > 1000) permille = 1000;
    if (permille < (centered ? -1000 : 0)) permille = centered ? -1000 : 0;
    if (permille == value && fillRole == role) return;
    value = permille; role = fillRole;
    invalidate();
}
void Bar::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    fill(r, clip, col(track));
    if (centered) {
        const int mid = r.x + r.width / 2;
        const int len = (r.width / 2) * (value < 0 ? -value : value) / 1000;
        if (value < 0) fill(Rect(mid - len, r.y, len, r.height), clip, col(role));
        else fill(Rect(mid, r.y, len, r.height), clip, col(role));
        fill(Rect(mid - 1, r.y - 0, 2, r.height), clip, col(ui::MUTED));
    } else {
        fill(Rect(r.x, r.y, r.width * value / 1000, r.height), clip, col(role));
    }
}

void Level::set(int permille, ui::Role fillRole, ui::Role border)
{
    if (permille < 0) permille = 0;
    if (permille > 1000) permille = 1000;
    if (permille == value && fillRole == role && border == outline) return;
    value = permille; role = fillRole; outline = border;
    invalidate();
}
void Level::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    const colortype surface = col(ui::SURFACE);
    ui::drawRounded(r, clip, col(outline), col(ui::RAISED), surface, 6);
    const int inner = r.height - 6;
    const int h = inner * value / 1000;
    if (h > 0) fill(Rect(r.x + 3, r.bottom() - 3 - h, r.width - 6, h), clip, mix(col(ui::RAISED), col(role), 54));
}

void Segments::set(int permille, ui::Role fillRole)
{
    if (permille < 0) permille = 0;
    if (permille > 1000) permille = 1000;
    if (permille == value && fillRole == role) return;
    value = permille; role = fillRole; invalidate();
}
void Segments::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    const int gap = 3;
    const int w = (r.width - gap * (count - 1)) / count;
    const int lit = value * count;  // permille * count
    for (int i = 0; i < count; ++i) {
        const Rect s(r.x + i * (w + gap), r.y, w, r.height);
        const int fullAt = (i + 1) * 1000, partAt = i * 1000;
        if (lit >= fullAt) fill(s, clip, col(role));
        else if (lit > partAt) { fill(s, clip, col(ui::RAISED)); fill(s, clip, col(role), 120); }
        else fill(s, clip, col(ui::RAISED));
    }
}

void Dots::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    int x = r.x;
    for (int i = 0; i < count; ++i) {
        const int w = i == index ? 16 : 6;
        fill(Rect(x, r.y + 1, w, 6), clip, col(i == index ? ui::ACCENT : ui::BORDER));
        x += w + 4;
    }
}


// ---------------------------------------------------------------- Btn
Btn::Btn() : tag(0), style(NORMAL), font(BODYM), detailFont(M14), icon(ICON_NONE), base(ui::BACKGROUND), ink(-1),
             pressed(false), selected(false), enabled(true), iconLeft(false), titleInk(-1), iconSize(20), align(CENTER), action(0)
{
    label[0] = detail[0] = 0;
    setTouchable(true);
}
void Btn::setup(int x, int y, int w, int h, const char* text, Style s, Font f)
{
    setPosition(x, y, w, h);
    style = s; font = f;
    put(label, 40, text);
}
void Btn::setLabel(const char* text)
{
    if (same(label, 40, text)) return;
    put(label, 40, text);
    invalidate();
}
void Btn::setDetail(const char* text, Font f)
{
    detailFont = f;
    if (same(detail, 40, text)) return;
    put(detail, 40, text);
    invalidate();
}
void Btn::handleClickEvent(const touchgfx::ClickEvent& e)
{
    if (!enabled) return;
    if (e.getType() == touchgfx::ClickEvent::PRESSED) { pressed = true; invalidate(); }
    else if (e.getType() == touchgfx::ClickEvent::RELEASED) {
        // Generous release area: small fingers slide while lifting.
        const bool fire = pressed && Rect(-24, -24, getWidth() + 48, getHeight() + 48).intersect(e.getX(), e.getY());
        pressed = false; invalidate();
        if (fire && action && action->isValid()) action->execute(*this);
    } else if (e.getType() == touchgfx::ClickEvent::CANCEL) { pressed = false; invalidate(); }
}
void Btn::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    colortype surface = col(ui::RAISED), border = col(ui::BORDER), text = col(ui::TEXT);
    switch (style) {
    case PRIMARY: surface = border = col(ui::ACCENT); text = col(ui::ON_ACCENT); break;
    case DANGER: surface = col(ui::SURFACE); border = text = col(ui::NEGATIVE); break;
    case DANGER_FILL: surface = border = col(ui::NEGATIVE); text = col(ui::ON_ACCENT); break;
    case POSITIVE_FILL: surface = border = col(ui::POSITIVE); text = col(ui::ON_ACCENT); break;
    case OUTLINE: surface = col(ui::SURFACE); border = text = col(ui::ACCENT); break;
    case GHOST: surface = col(base); border = col(base); text = col(ui::MUTED); break;
    case SEGMENT: surface = col(ui::RAISED); border = col(ui::RAISED); text = col(ui::MUTED); break;
    case FIELD: surface = col(ui::RAISED); border = col(ui::BORDER); break;
    case SWATCH: surface = border = ui::Theme::accentColor(tag & 3); break;
    case KEY: default: break;
    }
    if (selected) {
        if (style == SWATCH) border = col(ui::TEXT);
        else if (style == SEGMENT) { surface = border = col(ui::ACCENT); text = col(ui::ON_ACCENT); }
        else { border = col(ui::ACCENT); surface = col(ui::ACCENT_SOFT); }
    }
    if (pressed) surface = mix(surface, col(ui::TEXT), 10);
    if (!enabled) { text = col(ui::MUTED); if (style == PRIMARY || style == POSITIVE_FILL) { surface = border = col(ui::RAISED); } }
    if (ink >= 0) text = col(static_cast<ui::Role>(ink));
    const int radius = r.height <= 40 ? 6 : 8;
    ui::drawRounded(r, clip, border, surface, col(base), radius);
    if (icon != ICON_NONE && !label[0]) { drawIcon(icon, r, clip, text, surface, iconSize); return; }
    if (detail[0]) {
        const touchgfx::Font* top = touchgfx::TypedText(textId(font, LEFT)).getFont();
        const touchgfx::Font* bottom = touchgfx::TypedText(textId(detailFont, LEFT)).getFont();
        const int total = top->getHeight() + bottom->getHeight();
        const int y0 = r.y + (r.height - total) / 2;
        int tx = r.x + 12;
        if (icon != ICON_NONE) {
            drawIcon(icon, Rect(r.x + 12, r.y, 24, r.height), clip, col(ui::ACCENT), surface, 22);
            tx = r.x + 48;
        }
        const colortype titleColor = selected ? col(ui::ACCENT) : titleInk >= 0 ? col(static_cast<ui::Role>(titleInk)) : col(ui::MUTED);
        // Same inner padding on the right as on the left.
        drawText(label, font, LEFT, Rect(tx, y0, r.right() - tx - 12, top->getHeight()), clip, titleColor);
        drawText(detail, detailFont, LEFT, Rect(tx, y0 + top->getHeight(), r.right() - tx - 12, bottom->getHeight()), clip, text);
        return;
    }
    Rect box = r;
    if (icon != ICON_NONE && iconLeft) {
        const int tw = textWidth(label, font);
        const int x = r.x + (r.width - tw - 26) / 2;
        drawIcon(icon, Rect(x, r.y, 18, r.height), clip, text, surface, 16);
        drawText(label, font, LEFT, Rect(x + 26, r.y, tw + 4, r.height), clip, text);
        return;
    }
    if (icon != ICON_NONE) {
        drawIcon(icon, Rect(r.x + 10, r.y, 24, r.height), clip, align == CENTER ? text : col(ui::ACCENT), surface, align == CENTER ? 18 : 22);
        box = Rect(r.x + (align == CENTER ? 26 : 46), r.y, r.width - (align == CENTER ? 30 : 50), r.height);
    } else if (align != CENTER) box = Rect(r.x + 10, r.y, r.width - 20, r.height);
    drawText(label, font, align, box, clip, text);
}

// ---------------------------------------------------------------- Slider
void Slider::set(int permille)
{
    if (permille < 0) permille = 0;
    if (permille > 1000) permille = 1000;
    if (permille != value) { value = permille; invalidate(); }
}
void Slider::track(int x, bool final)
{
    const int knob = getHeight() / 2;
    const int span = getWidth() - 2 * knob;
    set(span > 0 ? (x - knob) * 1000 / span : 0);
    if (changed && changed->isValid()) changed->execute(*this, final);
}
void Slider::handleClickEvent(const touchgfx::ClickEvent& e)
{
    if (e.getType() == touchgfx::ClickEvent::PRESSED) { dragging = true; track(e.getX(), false); }
    else if (e.getType() == touchgfx::ClickEvent::RELEASED) { if (dragging) track(e.getX(), true); dragging = false; }
    else dragging = false;
}
void Slider::handleDragEvent(const touchgfx::DragEvent& e)
{
    if (dragging) track(e.getNewX(), false); // event coordinates are widget-relative
}
void Slider::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    const int knob = r.height / 2, cy = r.y + r.height / 2;
    const int span = r.width - 2 * knob;
    const int kx = r.x + knob + span * value / 1000;
    fill(Rect(r.x + knob, cy - 3, span, 6), clip, col(ui::RAISED));
    fill(Rect(r.x + knob, cy - 3, kx - r.x - knob, 6), clip, col(ui::ACCENT));
    ui::drawRounded(Rect(kx - 12, cy - 12, 24, 24), clip, col(ui::BORDER), col(ui::TEXT), col(ui::SURFACE), 12);
}

// ---------------------------------------------------------------- ThemeSwatch
void ThemeSwatch::handleClickEvent(const touchgfx::ClickEvent& e)
{
    if (e.getType() == touchgfx::ClickEvent::RELEASED && action && action->isValid()) action->execute(*this);
}
void ThemeSwatch::draw(const Rect& area) const
{
    // Tile: preview of the theme (its own colours) above the name and a note.
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    using ui::Theme;
    static const char* const names[] = { "Instrument", "Phosphor", "Light lab", "VFD amber" };
    static const char* const notes[] = { "navy \xC2\xB7 default", "scope green", "daylight bench", "retro display" };
    const colortype outline = selected ? col(ui::ACCENT) : col(ui::BORDER);
    const colortype tile = selected ? col(ui::ACCENT_SOFT) : col(ui::RAISED);
    ui::drawRounded(r, clip, outline, tile, col(ui::SURFACE), 8);
    if (selected) ui::drawRounded(Rect(r.x + 1, r.y + 1, r.width - 2, r.height - 2), clip, outline, tile, outline, 8);
    const Rect preview(r.x + 6, r.y + 6, r.width - 12, 60);
    ui::drawRounded(preview, clip, Theme::colorFor(theme, ui::BORDER), Theme::colorFor(theme, ui::BACKGROUND), tile, 6);
    touchgfx::Unicode::UnicodeChar t[16];
    put(t, 16, "12.004");
    drawText(t, M20, LEFT, Rect(preview.x + 8, preview.y + 4, preview.width - 12, 26), clip, Theme::colorFor(theme, ui::VOLT));
    put(t, 16, "0.842 A");
    drawText(t, M12, LEFT, Rect(preview.x + 8, preview.y + 30, preview.width - 12, 16), clip, Theme::colorFor(theme, ui::AMP));
    fill(Rect(preview.x + 8, preview.bottom() - 9, (preview.width - 16) * 2 / 3, 3), clip, Theme::colorFor(theme, ui::ACCENT));
    put(t, 16, names[theme & 3]);
    drawText(t, BODYM, LEFT, Rect(r.x + 8, r.y + 70, r.width - 12, 20), clip, col(ui::TEXT));
    put(t, 16, notes[theme & 3]);
    drawText(t, SUB, LEFT, Rect(r.x + 8, r.y + 90, r.width - 12, 16), clip, col(ui::MUTED));
}

// ---------------------------------------------------------------- Chip
Chip::Chip() : led(-1), ink(-1) { text[0] = 0; setHeight(24); }
int Chip::naturalWidth() const { return textWidth(text, CAP) + (led >= 0 ? 30 : 18); }
void Chip::set(const char* utf8, int ledRole, int inkRole)
{
    if (same(text, 32, utf8) && ledRole == led && inkRole == ink) return;
    invalidate();
    put(text, 32, utf8);
    led = ledRole; ink = inkRole;
    setWidth(naturalWidth());
    invalidate();
}
void Chip::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    ui::drawRounded(r, clip, col(ui::RAISED), col(ui::RAISED), col(ui::SURFACE), 12);
    int x = r.x + 9;
    if (led >= 0) {
        ui::drawRounded(Rect(x, r.y + 8, 8, 8), clip, col(static_cast<ui::Role>(led)), col(static_cast<ui::Role>(led)), col(ui::RAISED), 5);
        x += 13;
    }
    drawText(text, CAP, LEFT, Rect(x, r.y, r.right() - x - 6, r.height), clip, col(ink >= 0 ? static_cast<ui::Role>(ink) : ui::TEXT));
}

// ---------------------------------------------------------------- Plot
Plot::Plot() : cursor(-1), gridRows(3)
{
    count[0] = count[1] = 0;
    lo[0] = lo[1] = 0; hi[0] = hi[1] = 1;
    role[0] = ui::VOLT; role[1] = ui::AMP;
    dashed[0] = dashed[1] = false;
}
void Plot::setSeries(int s, const int16_t* v, int n, int16_t minimum, int16_t maximum, ui::Role r, bool dash)
{
    if (s < 0 || s > 1) return;
    if (n > MAX_POINTS) n = MAX_POINTS;
    memcpy(values[s], v, n * sizeof(int16_t));
    count[s] = n; lo[s] = minimum; hi[s] = maximum > minimum ? maximum : minimum + 1;
    role[s] = r; dashed[s] = dash;
    invalidate();
}
int Plot::yFor(int s, int16_t v) const
{
    const int h = getHeight() - 4;
    int y = 2 + h - (int)((int32_t)(v - lo[s]) * h / (hi[s] - lo[s]));
    if (y < 1) y = 1;
    if (y > getHeight() - 3) y = getHeight() - 3;
    return y;
}
void Plot::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    for (int i = 1; i <= gridRows; ++i) fill(Rect(r.x, r.y + r.height * i / (gridRows + 1), r.width, 1), clip, col(ui::BORDER));
    for (int s = 1; s >= 0; --s) {
        const int n = count[s];
        if (n < 2) continue;
        const colortype ink = col(role[s]);
        int prev = yFor(s, values[s][0]);
        for (int x = 0; x < r.width; ++x) {
            const int idx = x * (n - 1) / (r.width - 1);
            const int y = yFor(s, values[s][idx]);
            if (dashed[s] && (x / 5) % 2) { prev = y; continue; }
            const int top = y < prev ? y : prev, bottom = y < prev ? prev : y;
            fill(Rect(r.x + x, r.y + top - 1, 1, bottom - top + 2), clip, ink);
            prev = y;
        }
    }
    if (cursor >= 0 && cursor < r.width) fill(Rect(r.x + cursor, r.y, 2, r.height), clip, col(ui::TEXT));
}

// ---------------------------------------------------------------- Header
namespace { Header* activeHeader = 0; }
Header::~Header() { if (activeHeader == this) activeHeader = 0; }
void Header::tickActive() { if (activeHeader) activeHeader->tick(); }
Header::Header() : home(false), external(0), armTicks(0), divider10(0),
    backCallback(this, &Header::onBack), powerCallback(this, &Header::onPower)
{
    groupText[0] = 0;
}
void Header::setup(const PageInfo& info)
{
    activeHeader = this;
    setPosition(0, 0, touchgfx::HAL::DISPLAY_WIDTH, 48);
    background.setup(0, 0, getWidth(), 47, ui::SURFACE);
    divider.setup(0, 47, getWidth(), 1, ui::BORDER);
    grab.setup(getWidth() / 2 - 18, 4, 36, 4, ui::BORDER);
    add(background); add(divider); add(grab);
    int x = 12;
    if (info.back) {
        back.setup(2, 2, 42, 44, "", Btn::GHOST);
        back.setBase(ui::SURFACE);
        back.setIcon(ICON_BACK);
        back.setAction(backCallback);
        add(back);
        x = 46;
    }
    title.setup(TITLE, LEFT, x, 5, 300, 24, ui::TEXT);
    title.set(info.title);
    if (info.count > 0) snprintf(groupText, sizeof(groupText), "%s · %d / %d", info.group, info.index + 1, info.count);
    else snprintf(groupText, sizeof(groupText), "%s", info.group ? info.group : "");
    sub.setup(SUB, LEFT, x, 28, 330, 16, ui::MUTED);
    sub.set(groupText);
    add(title); add(sub);
    if (info.count > 1) {
        dots.setup(x + 6 + 160, 20, info.count, info.index);
        // Dots sit after the title; keep them clear of long titles.
        const touchgfx::Font* f = touchgfx::TypedText(textId(TITLE)).getFont();
        int tw = f ? f->getStringWidth(title.text()) : 120;
        dots.setX(x + tw + 14);
        add(dots);
    }
    home = !info.back;
    add(output); add(battery);
    if (home) add(link);
    power.setup(getWidth() - 52, 6, 44, 36, "", Btn::DANGER);
    power.setBase(ui::SURFACE);
    power.setIcon(ICON_POWER);
    power.setAction(powerCallback);
    add(power);
    refreshStatus();
}
void Header::setSub(const char* utf8, ui::Role role)
{
    if (armTicks) return; // the power confirmation message has priority
    sub.set(utf8 && utf8[0] ? utf8 : groupText, role);
}
void Header::adoptSub(touchgfx::Screen& screen, touchgfx::TextArea& text)
{
    external = &text;
    screen.getRootContainer().remove(text);
    screen.getRootContainer().add(text);
    text.setPosition(sub.getX(), sub.getY(), 330, 16);
    retype(text, SUB);
    text.setVisible(true);
    sub.setVisible(false);
    text.invalidate();
}

void Header::layoutChips()
{
    int right = power.getX() - 8;
    battery.setX(right - battery.getWidth()); battery.setY(12);
    right = battery.getX() - 6;
    if (home) { link.setX(right - link.getWidth()); link.setY(12); output.setVisible(false); return invalidate(); }
    output.setX(right - output.getWidth()); output.setY(12);
    invalidate();
}
void Header::refreshStatus()
{
    PsuSnapshot s;
    psu_snapshot(&s);
    const bool on = s.output_confirmed;
    const char* mode = s.mode_cc ? "CC" : "CV";
    char text[32];
    if (s.shutdown_pending) snprintf(text, sizeof(text), "STOPPING");
    else if (!s.g0_connected) snprintf(text, sizeof(text), "NO LINK");
    else if (on) snprintf(text, sizeof(text), "OUT ON · %s", mode);
    else snprintf(text, sizeof(text), "OUT OFF");
    output.set(text, on ? ui::POSITIVE : (!s.g0_connected ? ui::CAUTION : ui::MUTED));
    TelemetryRecord gauge(G4_RECORD_BATT), pack(G4_RECORD_TB);
    int64_t soc = 0;
    const bool haveSoc = gauge.get("soc_permille", soc) || pack.get("soc_permille", soc);
    int64_t ma = 0, mv = 0;
    const bool flow = pack.get("i_pack_ma", ma) && pack.get("pack_mv", mv);
    const bool charging = flow && ma > 0;
    if (home && flow && haveSoc) {
        const int64_t mw = mv * ma / 1000, a = mw < 0 ? -mw : mw;
        snprintf(text, sizeof(text), "%ld%%  %s%ld.%ld W", (long)((soc + 5) / 10), mw < 0 ? "-" : "+", (long)(a / 1000), (long)(a % 1000 / 100));
    } else if (haveSoc) snprintf(text, sizeof(text), "%ld%%", (long)((soc + 5) / 10));
    else snprintf(text, sizeof(text), "BAT --");
    battery.set(text, home ? (charging ? ui::POSITIVE : -1) : -1, charging && home ? ui::POSITIVE : -1);
    if (home) {
        TelemetryRecord meter(G4_RECORD_T), pd(G4_RECORD_TC);
        const bool all = meter.fresh && pack.fresh && pd.fresh;
        link.set("G4 \xC2\xB7 BMS \xC2\xB7 PD", all ? ui::POSITIVE : meter.fresh ? ui::CAUTION : ui::NEGATIVE);
    }
    layoutChips();
}
void Header::tick()
{
    if (armTicks && --armTicks == 0) {
        power.setStyle(Btn::DANGER);
        sub.set(groupText, ui::MUTED);
        if (external) { sub.setVisible(false); sub.invalidate(); external->setVisible(true); external->invalidate(); }
    }
    if (++divider10 >= 15) { divider10 = 0; refreshStatus(); }
}
void Header::onBack(const Btn&)
{
    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->navigateBack();
}
void Header::onPower(const Btn&)
{
    // Two-step confirmation: a single accidental touch never powers down the unit.
    if (!armTicks) {
        armTicks = 180;
        power.setStyle(Btn::DANGER_FILL);
        if (external) { external->setVisible(false); external->invalidate(); sub.setVisible(true); }
        sub.set("Tap power again to switch the unit OFF", ui::NEGATIVE);
        return;
    }
    armTicks = 0;
    power.setStyle(Btn::DANGER);
    const bool ok = psu_app_power_shutdown();
    sub.set(ok ? "POWER OFF requested - wake with TS2" : "POWER OFF request failed", ok ? ui::CAUTION : ui::NEGATIVE);
}

}

namespace kit {
// ---------------------------------------------------------------- Value
void Value::set(const char* utf8, ui::Role role)
{
    if (!utf8) utf8 = "";
    if (role == ink && !strncmp(text, utf8, sizeof(text) - 1)) return;
    invalidate();
    strncpy(text, utf8, sizeof(text) - 1);
    text[sizeof(text) - 1] = 0;
    ink = role;
}
void Value::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    char number[32], unit[16];
    const char* space = strrchr(text, ' ');
    // Only split off short alphabetic units ("V", "mAh", "%", "°C").
    if (space && strlen(space + 1) <= 4 && space[1] && !(space[1] >= '0' && space[1] <= '9') && space[1] != '-') {
        size_t n = space - text;
        if (n > sizeof(number) - 1) n = sizeof(number) - 1;
        memcpy(number, text, n); number[n] = 0;
        strncpy(unit, space + 1, sizeof(unit) - 1); unit[sizeof(unit) - 1] = 0;
    } else { strncpy(number, text, sizeof(number) - 1); number[sizeof(number) - 1] = 0; unit[0] = 0; }
    touchgfx::Unicode::UnicodeChar a[32], b[16];
    put(a, 32, number); put(b, 16, unit);
    Font big = this->big, small = this->small;
    const int gap = unit[0] ? 4 : 0;
    int wa = textWidth(a, big), wb = unit[0] ? textWidth(b, small) : 0;
    if (wa + gap + wb > r.width) {
        // Shrink number and unit together until the pair fits the box.
        const int need = wa + gap + wb;
        while (wa + gap + wb > r.width && smaller(big) != big) {
            big = smaller(big); if (smaller(small) != small && small != M12) small = smaller(small);
            wa = textWidth(a, big); wb = unit[0] ? textWidth(b, small) : 0;
        }
        if (overflowHook) overflowHook(a, need, r.width, wa + gap + wb <= r.width);
    }
    const touchgfx::Font* fa = touchgfx::TypedText(textId(big, LEFT)).getFont();
    const touchgfx::Font* fb = touchgfx::TypedText(textId(small, LEFT)).getFont();
    int x = r.x;
    if (align == CENTER) x = r.x + (r.width - wa - gap - wb) / 2;
    else if (align == RIGHT) x = r.right() - wa - gap - wb;
    const int top = r.y + (r.height - fa->getHeight()) / 2;
    const int baseline = top + fa->getBaseline();
    drawText(a, big, LEFT, Rect(x, top, wa + 4, fa->getHeight()), clip, col(ink));
    if (unit[0]) {
        const int ut = baseline - fb->getBaseline();
        drawText(b, small, LEFT, Rect(x + wa + gap, ut, wb + 4, fb->getHeight()), clip, col(ink == ui::MUTED ? ui::MUTED : ui::MUTED));
    }
}

// ---------------------------------------------------------------- Rows
void Rows::set(const char* t, bool isFresh)
{
    if (!t) t = "";
    if (isFresh == fresh && !strncmp(text, t, sizeof(text) - 1)) return;
    strncpy(text, t, sizeof(text) - 1);
    text[sizeof(text) - 1] = 0;
    fresh = isFresh;
    invalidate();
}
void Rows::draw(const Rect& area) const
{
    Rect r = getAbsoluteRect(), clip = area;
    clip.x += r.x; clip.y += r.y;
    const char* line = text;
    int y = r.y;
    while (*line && y + rowHeight <= r.bottom() + 2) {
        const char* end = strchr(line, '\n');
        const size_t n = end ? (size_t)(end - line) : strlen(line);
        char label[96], value[64];
        label[0] = value[0] = 0;
        if (n > 20 && line[19] == ' ' && !monoPlain) {
            size_t ln = 19;
            while (ln && line[ln - 1] == ' ') --ln;
            if (ln > sizeof(label) - 1) ln = sizeof(label) - 1;
            memcpy(label, line, ln); label[ln] = 0;
            size_t vs = 20, vn = n - 20;
            if (vn > sizeof(value) - 1) vn = sizeof(value) - 1;
            memcpy(value, line + vs, vn); value[vn] = 0;
        } else {
            size_t ln = n < sizeof(label) - 1 ? n : sizeof(label) - 1;
            memcpy(label, line, ln); label[ln] = 0;
        }
        touchgfx::Unicode::UnicodeChar u[64];
        if (value[0]) {
            put(u, 64, label);
            drawText(u, BODY, LEFT, Rect(r.x, y, r.width, rowHeight), clip, col(ui::MUTED));
            ui::Role role = valueRole(value, fresh);
            if (plainUnits && (role == ui::VOLT || role == ui::AMP || role == ui::WATT)) role = ui::TEXT;
            put(u, 64, value);
            drawText(u, M14, RIGHT, Rect(r.x, y, r.width, rowHeight), clip, col(role));
        } else if (label[0]) {
            touchgfx::Unicode::UnicodeChar w[96];
            put(w, 96, label);
            drawText(w, monoPlain ? M12 : BODY, LEFT, Rect(r.x, y, r.width, rowHeight), clip, col(fresh ? ui::TEXT : ui::MUTED));
        }
        y += rowHeight;
        if (!end) break;
        line = end + 1;
    }
}

// ---------------------------------------------------------------- Card / Metric
void Card::setup(int x, int y, int w, int h, const char* caption, const char* noteText)
{
    setPosition(x, y, w, h);
    panel.setup(0, 0, w, h, ui::SURFACE, ui::BORDER, ui::BACKGROUND, 11);
    title.setup(CAP, LEFT, 13, 9, w - 26, 16, ui::MUTED);
    title.set(caption);
    note.setup(SUB, RIGHT, 13, 8, w - 26, 16, ui::MUTED);
    note.set(noteText ? noteText : "");
    add(panel); add(title); add(note);
}
void Metric::setup(int x, int y, int w, int h, const char* caption, Font number, Font unit)
{
    setPosition(x, y, w, h);
    panel.setup(0, 0, w, h, ui::SURFACE, ui::BORDER, ui::BACKGROUND, 11);
    title.setup(CAP, LEFT, 12, 9, w - 20, 16, ui::MUTED);
    title.set(caption);
    const int vh = touchgfx::TypedText(textId(number, LEFT)).getFont()->getHeight();
    val.setup(number, unit, LEFT, 11, 26, w - 18, vh + 2, ui::TEXT);
    detail.setup(SUB, LEFT, 12, 28 + vh, w - 20, 16, ui::MUTED);
    add(panel); add(title); add(val); add(detail);
}
}
