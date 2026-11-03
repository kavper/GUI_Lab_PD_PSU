#ifndef KIT_HPP
#define KIT_HPP
// Shared, theme-aware building blocks for the unified UI.
// Every widget reads its colours from ui::Theme at draw time, so a theme or
// accent change only needs an invalidate; no per-widget theme bookkeeping.
#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/Screen.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/Unicode.hpp>
#include <gui/common/UiTheme.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <stdint.h>

namespace kit {

using touchgfx::colortype;
using touchgfx::Rect;

enum Font { CAP, SUB, BODY, BODYM, LABEL, TITLE, HEAD, M12, M14, M16, M20, M26, M34, M52, M60 };
enum Align { LEFT, CENTER, RIGHT };

touchgfx::TypedTextId textId(Font font, Align align = LEFT);
// Next smaller typography of the same family (itself when none).
Font smaller(Font font);
// Largest size of the family that fits width (reports overflow to the hook).
Font fitFont(const touchgfx::Unicode::UnicodeChar* text, Font font, int width);
// Simulator audit: called whenever text needed shrinking (fitted) or is cut.
extern void (*overflowHook)(const touchgfx::Unicode::UnicodeChar* text, int need, int have, bool fitted);
// Re-type an existing Designer wildcard TextArea to a kit typography.
void retype(touchgfx::TextArea& text, Font font, Align align = LEFT);
inline colortype col(ui::Role role) { return ui::Theme::color(role); }
// Show/hide a Designer widget, keeping its themed surface in step.
void hide(touchgfx::Drawable& d);
void place(touchgfx::Drawable& d, int x, int y, int w, int h);
// Hide every Designer child of a screen (kept for logic, never shown).
void hideAll(touchgfx::Container& root);

// Horizontal page swipes are suppressed when a press starts inside these
// rectangles (sliders, horizontally dragged controls). Reset per screen.
void clearNoSwipe();
void addNoSwipe(const Rect& r);
bool swipeBlocked(int x, int y);

// ---------------------------------------------------------------- text
// Every kit widget is tracked while it exists, so the Designer layout can be
// matched to the live widgets on any target (no RTTI needed).
enum Kind { K_LABEL, K_PANEL, K_FILL, K_BAR, K_LEVEL, K_SEGMENTS, K_DOTS, K_VALUE, K_ROWS,
            K_BTN, K_SLIDER, K_SWATCH, K_CHIP, K_PLOT, K_NONE };
struct Tracked {
    Tracked(touchgfx::Drawable* d, Kind k);
    ~Tracked();
    touchgfx::Drawable* owner;
};
// Kind of a tracked kit widget, K_NONE for anything else.
Kind kindOf(const touchgfx::Drawable* d);

class Label : public touchgfx::TextAreaWithOneWildcard {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_LABEL};
    Label();
    void setup(Font font, Align align, int x, int y, int w, int h, ui::Role role = ui::TEXT);
    void set(const char* utf8);
    void set(const char* utf8, ui::Role role);
    void setRole(ui::Role role);
    ui::Role role() const { return ink; }
    Align alignment() const { return kalign; }
    void setFont(Font f) { kfont = f; invalidate(); setTypedText(touchgfx::TypedText(textId(f, kalign))); invalidate(); }
    void setUnicode(const touchgfx::Unicode::UnicodeChar* t);
    Font typography() const { return kfont; }
    const touchgfx::Unicode::UnicodeChar* text() const { return buffer; }
    virtual void draw(const Rect& area) const;
    static const uint16_t SIZE = 96;
private:
    touchgfx::Unicode::UnicodeChar buffer[SIZE];
    ui::Role ink;
    Font kfont;
    Align kalign;
};

// ---------------------------------------------------------------- surfaces
class Panel : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_PANEL};
    Panel() : fillRole(ui::SURFACE), borderRole(ui::BORDER), baseRole(ui::BACKGROUND), radius(10) {}
    void setup(int x, int y, int w, int h, ui::Role fill = ui::SURFACE, ui::Role border = ui::BORDER, ui::Role base = ui::BACKGROUND, int r = 10);
    void setBorder(ui::Role border);
    void setFill(ui::Role fill);
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(0, 0, getWidth(), getHeight()); }
private:
    ui::Role fillRole, borderRole, baseRole;
    int radius;
};

class Fill : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_FILL};
    Fill() : role(ui::BACKGROUND), alpha(255) {}
    void setup(int x, int y, int w, int h, ui::Role r, uint8_t a = 255) { setPosition(x, y, w, h); role = r; alpha = a; }
    void setRole(ui::Role r) { if (r != role) { role = r; invalidate(); } }
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return alpha == 255 ? Rect(0, 0, getWidth(), getHeight()) : Rect(); }
private:
    ui::Role role;
    uint8_t alpha;
};

// Horizontal bar: value in permille. Optional centre zero for signed values.
class Bar : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_BAR};
    Bar() : value(0), role(ui::ACCENT), track(ui::RAISED), centered(false) {}
    void setup(int x, int y, int w, int h, ui::Role fill = ui::ACCENT, ui::Role trackRole = ui::RAISED) { setPosition(x, y, w, h); role = fill; track = trackRole; }
    void setCentered(bool c) { centered = c; }
    void set(int permille, ui::Role fill);
    void set(int permille) { set(permille, role); }
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(0, 0, getWidth(), getHeight()); }
private:
    int value;
    ui::Role role, track;
    bool centered;
};

// Vertical level (cells, temperatures). Value in permille of its height.
class Level : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_LEVEL};
    Level() : value(0), role(ui::VOLT), outline(ui::BORDER) {}
    void setup(int x, int y, int w, int h) { setPosition(x, y, w, h); }
    void set(int permille, ui::Role fill, ui::Role border = ui::BORDER);
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(); }
private:
    int value;
    ui::Role role, outline;
};

// Segmented gauge (state of charge): count segments, value in permille.
class Segments : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_SEGMENTS};
    Segments() : value(0), count(10), role(ui::POSITIVE) {}
    void setup(int x, int y, int w, int h, int n = 10) { setPosition(x, y, w, h); count = n; }
    void set(int permille, ui::Role fill);
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(); }
private:
    int value, count;
    ui::Role role;
};

class Dots : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_DOTS};
    Dots() : count(0), index(0) {}
    void setup(int x, int y, int n, int i) { count = n; index = i; setPosition(x, y, n ? n * 10 + 6 : 0, 8); }
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(); }
private:
    int count, index;
};

enum Icon { ICON_NONE, ICON_BACK, ICON_POWER, ICON_CHECK, ICON_UP, ICON_DOWN, ICON_LOCK,
            ICON_OUTPUT, ICON_BATTERY, ICON_CELLS, ICON_USB, ICON_PATH, ICON_TEMP, ICON_SHIELD, ICON_LIST,
            ICON_SEQ, ICON_BOLT, ICON_DIAG, ICON_NET, ICON_THEME, ICON_SERVICE, ICON_FAN, ICON_MONITOR,
            ICON_PLUS, ICON_MINUS, ICON_DEL, ICON_DOT };
// Anti-aliased stroke icon centred in box; base is the colour behind it.
void drawIcon(Icon icon, const Rect& box, const Rect& clip, colortype ink, colortype base, int size = 20);

// Number with a smaller, muted unit ("12.004" + "V"), split at the last space.
class Value : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_VALUE};
    Value() : big(M34), small(M16), align(LEFT), ink(ui::TEXT) { text[0] = 0; }
    void setup(Font number, Font unit, Align a, int x, int y, int w, int h, ui::Role role = ui::TEXT)
    { big = number; small = unit; align = a; ink = role; setPosition(x, y, w, h); }
    void set(const char* utf8, ui::Role role);
    void set(const char* utf8) { set(utf8, ink); }
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(); }
private:
    char text[32];
    Font big, small;
    Align align;
    ui::Role ink;
};

// Key / value rows from "label<pad to 19>value\n" text (TelemetryRecord::append
// format). Lines without a value are drawn as plain body text.
class Rows : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_ROWS};
    Rows() : rowHeight(22), fresh(true), plainUnits(true), monoPlain(false) { text[0] = 0; }
    void setMonoPlain(bool m) { monoPlain = m; }
    void setup(int x, int y, int w, int h, int row = 22) { setPosition(x, y, w, h); rowHeight = row; }
    void set(const char* t, bool isFresh = true);
    void setPlainUnits(bool p) { plainUnits = p; }
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(); }
private:
    char text[600];
    int rowHeight;
    bool fresh, plainUnits, monoPlain;
};

// Card: rounded panel with an upper-case caption and an optional right note.
class Card : public touchgfx::Container {
public:
    Card() {}
    void setup(int x, int y, int w, int h, const char* caption, const char* note = 0);
    void setCaption(const char* t) { title.set(t); }
    void setNote(const char* t, ui::Role role = ui::MUTED) { note.set(t, role); }
    void setBorder(ui::Role r) { panel.setBorder(r); }
    void setFill(ui::Role r) { panel.setFill(r); }
private:
    Panel panel;
    Label title, note;
};

// Metric card: caption, big value with unit, optional detail line.
class Metric : public touchgfx::Container {
public:
    Metric() {}
    void setup(int x, int y, int w, int h, const char* caption, Font number = M26, Font unit = M14);
    void set(const char* value, ui::Role role) { val.set(value, role); }
    void setDetail(const char* t, ui::Role role = ui::MUTED) { detail.set(t, role); }
    void setBorder(ui::Role r) { panel.setBorder(r); }
private:
    Panel panel;
    Label title, detail;
    Value val;
};

// ---------------------------------------------------------------- controls
class Btn : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_BTN};
    enum Style { NORMAL, PRIMARY, DANGER, DANGER_FILL, POSITIVE_FILL, OUTLINE, GHOST, SEGMENT, KEY, FIELD, SWATCH };
    Btn();
    void setup(int x, int y, int w, int h, const char* label, Style style = NORMAL, Font font = BODYM);
    void setLabel(const char* utf8);
    void setStyle(Style s) { if (s != style) { style = s; invalidate(); } }
    void setSelected(bool s) { if (s != selected) { selected = s; invalidate(); } }
    void setEnabled(bool e) { if (e != enabled) { enabled = e; setTouchable(e); invalidate(); } }
    void setIcon(Icon i) { if (i != icon) { icon = i; invalidate(); } }
    void setIconLeft(bool left) { iconLeft = left; }
    void setTitleInk(int role) { if (role != titleInk) { titleInk = role; invalidate(); } }
    void setAlign(Align a) { align = a; }
    void setIconSize(int px) { iconSize = px; }
    void setInk(int role) { ink = role; invalidate(); }
    void setBase(ui::Role b) { base = b; }
    bool isSelected() const { return selected; }
    void setAction(touchgfx::GenericCallback<const Btn&>& cb) { action = &cb; }
    // Secondary text rendered under the label (fields, presets, tiles).
    void setDetail(const char* utf8, Font font = M14);
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(0, 0, getWidth(), getHeight()); }
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    int tag;
private:
    touchgfx::Unicode::UnicodeChar label[40];
    touchgfx::Unicode::UnicodeChar detail[40];
    Style style;
    Font font, detailFont;
    Icon icon;
    ui::Role base;
    int ink;
    bool pressed, selected, enabled, iconLeft;
    int titleInk, iconSize;
    Align align;
    touchgfx::GenericCallback<const Btn&>* action;
};

// Horizontal slider, value 0..1000. Reports changes while dragging and on release.
class Slider : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_SLIDER};
    Slider() : value(0), dragging(false), changed(0) { setTouchable(true); }
    void setup(int x, int y, int w, int h) { setPosition(x, y, w, h); }
    void set(int permille);
    int get() const { return value; }
    void setAction(touchgfx::GenericCallback<const Slider&, bool>& cb) { changed = &cb; }
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(); }
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleDragEvent(const touchgfx::DragEvent& event);
private:
    int value;
    bool dragging;
    touchgfx::GenericCallback<const Slider&, bool>* changed;
    void track(int x, bool final);
};

// Miniature of a theme: background, card, and the V / A / accent colours.
class ThemeSwatch : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_SWATCH};
    ThemeSwatch() : theme(0), selected(false), action(0) { setTouchable(true); }
    void setup(int x, int y, int w, int h, unsigned t) { setPosition(x, y, w, h); theme = t; }
    void setSelected(bool s) { if (s != selected) { selected = s; invalidate(); } }
    unsigned themeIndex() const { return theme; }
    void setAction(touchgfx::GenericCallback<const ThemeSwatch&>& cb) { action = &cb; }
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(); }
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
private:
    unsigned theme;
    bool selected;
    touchgfx::GenericCallback<const ThemeSwatch&>* action;
};

// Rounded status pill with optional LED dot. Width follows the text.
class Chip : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_CHIP};
    Chip();
    void set(const char* utf8, int ledRole = -1, int inkRole = -1);
    int naturalWidth() const;
    // Chips are re-anchored after their text (and width) changes: redraw both
    // the old and the new place, otherwise a stale copy stays on screen.
    virtual void setX(int16_t x) { if (x != getX()) { invalidate(); touchgfx::Widget::setX(x); invalidate(); } }
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(); }
private:
    touchgfx::Unicode::UnicodeChar text[32];
    int led, ink;
};

// Line plot of up to two series, optional step rendering and cursor.
class Plot : public touchgfx::Widget {
public:
    friend struct LayoutAccess;
    Tracked kitTracked{this, K_PLOT};
    static const int MAX_POINTS = 160;
    Plot();
    void setup(int x, int y, int w, int h) { setPosition(x, y, w, h); }
    void clear() { count[0] = count[1] = 0; invalidate(); }
    void setSeries(int series, const int16_t* values, int n, int16_t minimum, int16_t maximum, ui::Role role, bool dashed = false);
    void setCursor(int px) { cursor = px; invalidate(); }
    void setGrid(int rows) { gridRows = rows; }
    virtual void draw(const Rect& area) const;
    virtual Rect getSolidRect() const { return Rect(); }
private:
    int16_t values[2][MAX_POINTS];
    int count[2];
    int16_t lo[2], hi[2];
    ui::Role role[2];
    bool dashed[2];
    int cursor, gridRows;
    int yFor(int series, int16_t v) const;
};

// ---------------------------------------------------------------- header
struct PageInfo { const char* title; const char* group; int index; int count; bool back; };

class Header : public touchgfx::Container {
public:
    Header();
    virtual ~Header();
    void setup(const PageInfo& info);
    // Ticked by the application for the header of the current screen.
    static void tickActive();
    // Status/notice line under the title (replaces per-screen PageFeedback).
    void setSub(const char* utf8, ui::Role role = ui::MUTED);
    // Use a view's existing status TextArea as the subtitle (call after add(header)).
    void adoptSub(touchgfx::Screen& screen, touchgfx::TextArea& text);
    void tick();
    void refreshStatus();
private:
    Fill background, divider, grab;
    Btn back, power;
    Label title, sub;
    Dots dots;
    Chip output, battery, link;
    bool home;
    char groupText[40];
    touchgfx::TextArea* external;
    uint16_t armTicks, divider10;
    touchgfx::Callback<Header, const Btn&> backCallback, powerCallback;
    void onBack(const Btn&);
    void onPower(const Btn&);
    void layoutChips();
};

}
#endif
