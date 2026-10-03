#ifndef UITHEME_HPP
#define UITHEME_HPP
#include <touchgfx/Color.hpp>
#include <touchgfx/Screen.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/widgets/ButtonWithLabel.hpp>
#include <touchgfx/widgets/TextArea.hpp>

namespace ui {
enum Role { BACKGROUND, SURFACE, RAISED, BORDER, TEXT, MUTED, ACCENT, ACCENT_SOFT, POSITIVE, CAUTION, NEGATIVE, ON_ACCENT };
enum SurfaceStyle { PANEL, NORMAL, PRIMARY, DANGER, OUTPUT, PILL, SELECTION, ICON, SWATCH_BLUE, SWATCH_TEAL, SWATCH_VIOLET, SWATCH_AMBER };
class Theme {
public:
    static bool dark();
    static unsigned accentIndex();
    static void setDark(bool value);
    static void setAccent(unsigned value);
    static touchgfx::colortype color(Role role);
    static touchgfx::colortype accentColor(unsigned index);
    static touchgfx::colortype translate(touchgfx::colortype color);
    static const char* accentName();
};

// Covers the bitmap surface while preserving the original widget's callbacks,
// visibility, enabled state and dynamic text. It owns no image/frame buffer.
class ThemedSurface : public touchgfx::Widget {
public:
    ThemedSurface();
    void bind(touchgfx::Drawable& original, SurfaceStyle style, touchgfx::Button* button=0, touchgfx::ButtonWithLabel* label=0, touchgfx::Image* image=0);
    bool sync();
    virtual touchgfx::Rect getSolidRect() const;
    virtual void draw(const touchgfx::Rect& area) const;
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleDragEvent(const touchgfx::DragEvent& event);
private:
    touchgfx::Drawable* original;
    touchgfx::Button* button;
    touchgfx::ButtonWithLabel* label;
    touchgfx::Image* image;
    SurfaceStyle style;
    uint16_t lastBitmap;
    uint8_t lastAlpha;
    bool selected() const;
};

class ThemeScreen {
public:
    ThemeScreen();
    static ThemeScreen& get();
    void begin(touchgfx::Screen& screen);
    void detach();
    void text(touchgfx::TextArea& widget, int fixedRole=-1);
    void box(touchgfx::Box& widget, Role role);
    void panel(touchgfx::Box& widget);
    void button(touchgfx::Button& widget, SurfaceStyle style=NORMAL);
    void button(touchgfx::ButtonWithLabel& widget, SurfaceStyle style=NORMAL);
    void image(touchgfx::Image& widget, SurfaceStyle style, touchgfx::Button* tile=0);
    void sync();
    void apply();
private:
    struct TextBinding { touchgfx::TextArea* widget; int8_t role; };
    struct BoxBinding { touchgfx::Box* widget; Role role; };
    touchgfx::Screen* screen;
    ThemedSurface surfaces[48];
    TextBinding texts[64];
    BoxBinding boxes[32];
    uint8_t surfaceCount,textCount,boxCount;
    void add(touchgfx::Drawable& widget,SurfaceStyle style,touchgfx::Button* button,touchgfx::ButtonWithLabel* label,touchgfx::Image* image);
};
}
#endif
