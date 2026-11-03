#include <gui/common/KitLayout.hpp>
#include <gui/common/Kit.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextArea.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/hal/HAL.hpp>
#ifdef SIMULATOR
#include <gui/common/UiTheme.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/widgets/AbstractButton.hpp>
#endif
#include <string.h>
#include <stdlib.h>

namespace kit {

// ---------------------------------------------------------------- tracking
namespace {
const unsigned TRACK_MAX = 1024;
touchgfx::Drawable* trackedPtr[TRACK_MAX];
uint8_t trackedKind[TRACK_MAX];
unsigned trackedTop = 0;
}

Tracked::Tracked(touchgfx::Drawable* d, Kind k) : owner(d)
{
    for (unsigned i = 0; i < trackedTop; ++i)
        if (!trackedPtr[i]) { trackedPtr[i] = d; trackedKind[i] = k; return; }
    if (trackedTop < TRACK_MAX) { trackedPtr[trackedTop] = d; trackedKind[trackedTop] = k; ++trackedTop; }
}
Tracked::~Tracked()
{
    for (unsigned i = 0; i < trackedTop; ++i)
        if (trackedPtr[i] == owner) { trackedPtr[i] = 0; break; }
    while (trackedTop && !trackedPtr[trackedTop - 1]) --trackedTop;
}
Kind kindOf(const touchgfx::Drawable* d)
{
    for (unsigned i = 0; i < trackedTop; ++i)
        if (trackedPtr[i] == d) return static_cast<Kind>(trackedKind[i]);
    return K_NONE;
}

namespace {
const unsigned MAX_ENTRIES = 400;

// Kit widgets of a page in a fixed order: depth first through containers.
void collect(touchgfx::Drawable* parent, touchgfx::Drawable** out, unsigned& n)
{
    for (touchgfx::Drawable* d = parent->getFirstChild(); d; d = d->getNextSibling()) {
        if (kindOf(d) != K_NONE) { if (n < MAX_ENTRIES) out[n] = d; ++n; }
        else if (d->getFirstChild()) collect(d, out, n);
    }
}

int fontIndex(const touchgfx::Font* f)
{
    for (int i = CAP; i <= M60; ++i)
        if (touchgfx::TypedText(textId(static_cast<Font>(i), LEFT)).getFont() == f) return i;
    return -1;
}

bool shown(const touchgfx::Drawable* d)
{
    for (; d; d = d->getParent()) if (!d->isVisible()) return false;
    return true;
}

// The Layout container starts with an invisible 1x1 marker box.
bool isLayout(touchgfx::Drawable* d)
{
    touchgfx::Drawable* first = d->getFirstChild();
    if (!first || first->isVisible() || first->getWidth() != 1 || first->getHeight() != 1) return false;
    if (d->getX() || d->getY() || d->getWidth() != touchgfx::HAL::DISPLAY_WIDTH || d->getHeight() != touchgfx::HAL::DISPLAY_HEIGHT) return false;
    const touchgfx::colortype c = static_cast<touchgfx::Box*>(first)->getColor();
    return touchgfx::Color::getRed(c) == 40 && touchgfx::Color::getGreen(c) == 80 && touchgfx::Color::getBlue(c) == 120;
}
}

uint32_t textHash(const touchgfx::Unicode::UnicodeChar* t)
{
    uint32_t h = 2166136261u;
    while (t && *t) { h ^= *t++; h *= 16777619u; }
    return h;
}

// ---------------------------------------------------------------- access
struct LayoutAccess {
    static Font labelFont(const Label& l) { return l.kfont; }
    static const touchgfx::Unicode::UnicodeChar* btnLabel(const Btn& b) { return b.label; }
    static const touchgfx::Unicode::UnicodeChar* btnDetail(const Btn& b) { return b.detail; }
    static Font btnFont(const Btn& b) { return b.font; }
    static Font btnDetailFont(const Btn& b) { return b.detailFont; }
    static int btnIcon(const Btn& b) { return b.icon; }
    static int btnIconSize(const Btn& b) { return b.iconSize; }
    static bool btnIconLeft(const Btn& b) { return b.iconLeft; }
    static int btnAlign(const Btn& b) { return b.align; }
    static colortype btnText(const Btn& b)
    {
        colortype text = col(ui::TEXT);
        switch (b.style) {
        case Btn::PRIMARY: case Btn::DANGER_FILL: case Btn::POSITIVE_FILL: text = col(ui::ON_ACCENT); break;
        case Btn::DANGER: text = col(ui::NEGATIVE); break;
        case Btn::OUTLINE: text = col(ui::ACCENT); break;
        case Btn::GHOST: case Btn::SEGMENT: text = col(ui::MUTED); break;
        default: break;
        }
        if (b.selected && b.style == Btn::SEGMENT) text = col(ui::ON_ACCENT);
        if (!b.enabled) text = col(ui::MUTED);
        if (b.ink >= 0) text = col(static_cast<ui::Role>(b.ink));
        return text;
    }
    static colortype btnTitle(const Btn& b)
    {
        return b.selected ? col(ui::ACCENT) : b.titleInk >= 0 ? col(static_cast<ui::Role>(b.titleInk)) : col(ui::MUTED);
    }
    static const char* valueText(const Value& v) { return v.text; }
    static Font valueFont(const Value& v) { return v.big; }
    static colortype valueInk(const Value& v) { return col(v.ink); }
    static int valueAlign(const Value& v) { return v.align; }
    static const char* rowsText(const Rows& r) { return r.text; }
    static const touchgfx::Unicode::UnicodeChar* chipText(const Chip& c) { return c.text; }
    static int chipLed(const Chip& c) { return c.led; }
    static colortype chipInk(const Chip& c) { return col(c.ink >= 0 ? static_cast<ui::Role>(c.ink) : ui::TEXT); }
    static Font valueSmall(const Value& v) { return v.small; }
    static int rowsHeight(const Rows& r) { return r.rowHeight; }
    static void bar(const Bar& b, int& v, colortype& fill, colortype& track, bool& centered) { v = b.value; fill = col(b.role); track = col(b.track); centered = b.centered; }
    static int levelValue(const Level& l) { return l.value; }
    static colortype levelOutline(const Level& l) { return col(l.outline); }
    static void segments(const Segments& s, int& v, int& n, colortype& fill) { v = s.value; n = s.count; fill = col(s.role); }
    static void dots(const Dots& d, int& n, int& i) { n = d.count; i = d.index; }
    static int sliderValue(const Slider& s) { return s.value; }
};

// ---------------------------------------------------------------- binding
int applyDesignerLayout(unsigned page, touchgfx::Container& root)
{
    if (page >= layoutScreenCount || !layoutScreens[page].count) return -1;
    const LayoutScreen& screen = layoutScreens[page];
    touchgfx::Drawable* layout = 0;
    for (touchgfx::Drawable* d = root.getFirstChild(); d; d = d->getNextSibling())
        if (d->getFirstChild() && isLayout(d)) layout = d;
    if (!layout) return -1;
    layout->setVisible(false);

    static touchgfx::Drawable* widgets[MAX_ENTRIES];
    unsigned n = 0;
    collect(&root, widgets, n);
    if (n != screen.count) return -1;
    static touchgfx::Drawable* nodes[MAX_ENTRIES];
    unsigned m = 0;
    // Skip the marker and the page background box.
    touchgfx::Drawable* first = layout->getFirstChild()->getNextSibling();
    if (first) first = first->getNextSibling();
    for (touchgfx::Drawable* d = first; d; d = d->getNextSibling()) {
        if (m < MAX_ENTRIES) nodes[m] = d;
        ++m;
    }
    if (m != n) return -1;
    for (unsigned i = 0; i < n; ++i) if (kindOf(widgets[i]) != screen.entries[i].kind) return -1;

    int changed = 0;
    for (unsigned i = 0; i < n; ++i) {
        const LayoutEntry& e = screen.entries[i];
        touchgfx::Drawable* w = widgets[i];
        touchgfx::Drawable* d = nodes[i];
        bool touched = false;
        if (d->getX() != e.x || d->getY() != e.y || d->getWidth() != e.w || d->getHeight() != e.h) {
            touchgfx::Rect parent;
            if (w->getParent() && w->getParent() != &root) parent = w->getParent()->getAbsoluteRect();
            w->invalidate();
            w->setPosition(d->getX() - parent.x, d->getY() - parent.y, d->getWidth(), d->getHeight());
            w->invalidate();
            touched = true;
        }
        if (d->isVisible() != (e.visible != 0)) { w->invalidate(); w->setVisible(d->isVisible()); w->invalidate(); touched = true; }
        const touchgfx::TextArea* text = 0;
        if (e.kind == K_LABEL) text = static_cast<const touchgfx::TextArea*>(d);
        else if (e.kind == K_BTN) {
            // Button: 1-2 boxes (bits 0-1), icon Image (bit 7), label TextArea
            // when it has one (bit 6), optional detail.
            touchgfx::Drawable* c = d->getFirstChild();
            for (int skip = (e.font & 3) + ((e.font & 0x80) ? 1 : 0); c && skip; --skip) c = c->getNextSibling();
            if (c && (e.font & 0x40)) text = static_cast<const touchgfx::TextArea*>(c);
        }
        if (text && text->getTypedText().hasValidId()) {
            const touchgfx::Unicode::UnicodeChar* t = text->getTypedText().getText();
            if (textHash(t) != e.hash) {
                if (e.kind == K_LABEL) static_cast<Label*>(w)->setUnicode(t);
                else { char utf8[80]; touchgfx::Unicode::toUTF8(t, reinterpret_cast<uint8_t*>(utf8), sizeof(utf8)); static_cast<Btn*>(w)->setLabel(utf8); }
                touched = true;
            }
            if (e.kind == K_LABEL) {
                const int f = fontIndex(text->getTypedText().getFont());
                if (f >= 0 && f != (e.font & 0x7f)) { static_cast<Label*>(w)->setFont(static_cast<Font>(f)); touched = true; }
            }
        }
#ifdef SIMULATOR
        if (touched && getenv("PSU_LAYOUT_DEBUG")) {
            char u[120] = ""; if (text && text->getTypedText().hasValidId()) touchgfx::Unicode::toUTF8(text->getTypedText().getText(), reinterpret_cast<uint8_t*>(u), sizeof(u));
            fprintf(stderr, "bind page %u #%u kind %d designer %d,%d %dx%d vis %d table %d,%d %dx%d vis %d text '%s' hash %u/%u\n", page, i, e.kind,
                    d->getX(), d->getY(), d->getWidth(), d->getHeight(), d->isVisible(), e.x, e.y, e.w, e.h, e.visible, u,
                    text ? (unsigned)textHash(text->getTypedText().getText()) : 0u, (unsigned)e.hash);
        }
#endif
        if (touched) ++changed;
    }
    return changed;
}

// ---------------------------------------------------------------- export
#ifdef SIMULATOR
namespace {
void rgb(FILE* f, const char* key, colortype c)
{
    fprintf(f, ",\"%s\":[%d,%d,%d]", key, touchgfx::Color::getRed(c), touchgfx::Color::getGreen(c), touchgfx::Color::getBlue(c));
}
void str(FILE* f, const char* key, const char* s)
{
    fprintf(f, ",\"%s\":\"", key);
    for (; s && *s; ++s) {
        if (*s == '"' || *s == '\\') fprintf(f, "\\%c", *s);
        else if (*s == '\n') fprintf(f, "\\n");
        else if ((unsigned char)*s < 0x20) continue;
        else fputc(*s, f);
    }
    fputc('"', f);
}
void ustr(FILE* f, const char* key, const touchgfx::Unicode::UnicodeChar* t)
{
    char utf8[400];
    touchgfx::Unicode::toUTF8(t, reinterpret_cast<uint8_t*>(utf8), sizeof(utf8));
    str(f, key, utf8);
}
colortype sample(const uint16_t* fb, int x, int y)
{
    if (x < 0) x = 0; if (y < 0) y = 0;
    if (x >= touchgfx::HAL::DISPLAY_WIDTH) x = touchgfx::HAL::DISPLAY_WIDTH - 1;
    if (y >= touchgfx::HAL::DISPLAY_HEIGHT) y = touchgfx::HAL::DISPLAY_HEIGHT - 1;
    const uint16_t v = fb[y * touchgfx::HAL::DISPLAY_WIDTH + x];
    return touchgfx::Color::getColorFromRGB((v >> 11) * 255 / 31, ((v >> 5) & 63) * 255 / 63, (v & 31) * 255 / 31);
}
}

// Designer items that stay in use (placed/retyped by the page code): their
// child index path, runtime rectangle and visibility. Order matches Designer
// (generated code adds them in tree order after the screen background).
void exportLegacy(FILE* out, touchgfx::Drawable* parent, char* path, size_t len, bool& first)
{
    int index = 0;
    touchgfx::Drawable* previous = 0;
    for (touchgfx::Drawable* d = parent->getFirstChild(); d; d = d->getNextSibling()) {
        if (dynamic_cast<ui::ThemedSurface*>(d)) {
            if (d->isVisible() && previous) fprintf(out, ",\"themed\":1");
            continue;
        }
        if (kindOf(d) != K_NONE || dynamic_cast<Card*>(d) || dynamic_cast<Metric*>(d) || dynamic_cast<Header*>(d)) {
            if (previous) fputc('}', out);
            previous = 0; continue;
        }
        if (previous) fputc('}', out);
        const char* type = dynamic_cast<touchgfx::TextArea*>(d) ? "TextArea" : dynamic_cast<touchgfx::Image*>(d) ? "Image" :
                           dynamic_cast<touchgfx::Box*>(d) ? "Box" : dynamic_cast<touchgfx::AbstractButton*>(d) ? "Button" :
                           d->getFirstChild() ? "Container" : "Other";
        fprintf(out, "%s\n{\"path\":\"%s%d\",\"type\":\"%s\",\"visible\":%d,\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d",
                first ? "" : ",", path, index, type, d->isVisible() ? 1 : 0, d->getX(), d->getY(), d->getWidth(), d->getHeight());
        first = false;
        previous = d;
        if (d->getFirstChild() && strcmp(type, "Container") == 0) {
            fputc('}', out); previous = 0;
            char sub[64]; snprintf(sub, sizeof(sub), "%s%d.", path, index);
            exportLegacy(out, d, sub, sizeof(sub), first);
        }
        ++index;
    }
    if (previous) fputc('}', out);
    (void)len;
}

void exportLayout(FILE* out, unsigned page, const char* name, touchgfx::Container& root, const uint16_t* fb)
{
    static touchgfx::Drawable* widgets[MAX_ENTRIES];
    unsigned n = 0;
    collect(&root, widgets, n);
    if (n > MAX_ENTRIES) n = MAX_ENTRIES;
    fprintf(out, "{\"page\":%u,\"name\":\"%s\",\"background\":[%d,%d,%d],\"widgets\":[", page, name,
            touchgfx::Color::getRed(col(ui::BACKGROUND)), touchgfx::Color::getGreen(col(ui::BACKGROUND)), touchgfx::Color::getBlue(col(ui::BACKGROUND)));
    for (unsigned i = 0; i < n; ++i) {
        touchgfx::Drawable* w = widgets[i];
        const touchgfx::Rect r = w->getAbsoluteRect();
        const Kind k = kindOf(w);
        fprintf(out, "%s\n{\"kind\":%d,\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d,\"visible\":%d", i ? "," : "", (int)k, r.x, r.y, r.width, r.height, shown(w) ? 1 : 0);
        // Fill colour as drawn, sampled just inside the top edge.
        rgb(out, "fill", sample(fb, r.x + r.width / 2, r.y + (r.height > 6 ? 3 : r.height / 2)));
        switch (k) {
        case K_LABEL: {
            const Label& l = *static_cast<Label*>(w);
            ustr(out, "text", l.text()); fprintf(out, ",\"font\":%d,\"align\":%d", (int)LayoutAccess::labelFont(l), (int)l.alignment());
            rgb(out, "ink", col(l.role()));
            fprintf(out, ",\"hash\":%u", (unsigned)textHash(l.text()));
            break;
        }
        case K_BTN: {
            const Btn& b = *static_cast<Btn*>(w);
            ustr(out, "text", LayoutAccess::btnLabel(b)); ustr(out, "detail", LayoutAccess::btnDetail(b));
            fprintf(out, ",\"font\":%d,\"detailFont\":%d,\"icon\":%d,\"iconSize\":%d,\"iconLeft\":%d,\"align\":%d",
                    (int)LayoutAccess::btnFont(b), (int)LayoutAccess::btnDetailFont(b), LayoutAccess::btnIcon(b),
                    LayoutAccess::btnIconSize(b), LayoutAccess::btnIconLeft(b) ? 1 : 0, LayoutAccess::btnAlign(b));
            rgb(out, "ink", LayoutAccess::btnText(b)); rgb(out, "title", LayoutAccess::btnTitle(b));
            rgb(out, "border", sample(fb, r.x + r.width / 2, r.y));
            fprintf(out, ",\"hash\":%u", (unsigned)textHash(LayoutAccess::btnLabel(b)));
            break;
        }
        case K_VALUE: {
            const Value& v = *static_cast<Value*>(w);
            str(out, "text", LayoutAccess::valueText(v)); fprintf(out, ",\"font\":%d,\"small\":%d,\"align\":%d", (int)LayoutAccess::valueFont(v), (int)LayoutAccess::valueSmall(v), LayoutAccess::valueAlign(v));
            rgb(out, "muted", col(ui::MUTED));
            rgb(out, "ink", LayoutAccess::valueInk(v));
            break;
        }
        case K_ROWS:
            str(out, "text", LayoutAccess::rowsText(*static_cast<Rows*>(w))); rgb(out, "ink", col(ui::TEXT));
            rgb(out, "muted", col(ui::MUTED)); fprintf(out, ",\"row\":%d", LayoutAccess::rowsHeight(*static_cast<Rows*>(w)));
            break;
        case K_BAR: {
            int v; colortype a, t; bool c; LayoutAccess::bar(*static_cast<Bar*>(w), v, a, t, c);
            fprintf(out, ",\"value\":%d,\"centered\":%d", v, c ? 1 : 0); rgb(out, "role", a); rgb(out, "track", t); rgb(out, "muted", col(ui::MUTED));
            break;
        }
        case K_LEVEL: {
            const Level& l = *static_cast<Level*>(w);
            fprintf(out, ",\"value\":%d", LayoutAccess::levelValue(l));
            rgb(out, "outline", LayoutAccess::levelOutline(l)); rgb(out, "track", col(ui::RAISED));
            rgb(out, "role", sample(fb, r.x + r.width / 2, r.bottom() - 5));
            break;
        }
        case K_SEGMENTS: {
            int v, c; colortype a; LayoutAccess::segments(*static_cast<Segments*>(w), v, c, a);
            fprintf(out, ",\"value\":%d,\"count\":%d", v, c); rgb(out, "role", a); rgb(out, "track", col(ui::RAISED));
            break;
        }
        case K_DOTS: {
            int c, i; LayoutAccess::dots(*static_cast<Dots*>(w), c, i);
            fprintf(out, ",\"count\":%d,\"index\":%d", c, i); rgb(out, "role", col(ui::ACCENT)); rgb(out, "track", col(ui::BORDER));
            break;
        }
        case K_SLIDER:
            fprintf(out, ",\"value\":%d", LayoutAccess::sliderValue(*static_cast<Slider*>(w)));
            rgb(out, "role", col(ui::ACCENT)); rgb(out, "track", col(ui::RAISED)); rgb(out, "knob", col(ui::TEXT)); rgb(out, "outline", col(ui::BORDER));
            break;
        case K_CHIP: {
            const Chip& c = *static_cast<Chip*>(w);
            ustr(out, "text", LayoutAccess::chipText(c)); rgb(out, "ink", LayoutAccess::chipInk(c));
            if (LayoutAccess::chipLed(c) >= 0) rgb(out, "led", col(static_cast<ui::Role>(LayoutAccess::chipLed(c))));
            rgb(out, "fill", col(ui::RAISED));
            break;
        }
        case K_PANEL: rgb(out, "border", sample(fb, r.x + r.width / 2, r.y)); break;
        default: break;
        }
        fputc('}', out);
    }
    fprintf(out, "],\"legacy\":[");
    char path[4] = "";
    bool first = true;
    exportLegacy(out, &root, path, sizeof(path), first);
    fprintf(out, "]}\n");
    fflush(out);
}
#endif
}
