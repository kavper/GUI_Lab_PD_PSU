// Anti-aliased stroke icons drawn from a few vector primitives (24 x 24 design
// grid, like the SVG mockup) so no bitmaps are needed per theme.
#include <gui/common/Kit.hpp>
#include <math.h>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/Bitmap.hpp>
#include <touchgfx/hal/Config.hpp>
#ifndef SIMULATOR
extern "C" {
#include "main.h"
}
#endif

namespace kit {
namespace {
// Coordinates are in half units of the 24 px grid.
enum PrimType { END, LINE, RING, ARC, DISC, PIE };
struct Prim { uint8_t type; int16_t a, b, c, d, e; };

#define L(x1, y1, x2, y2) { LINE, (int16_t)((x1) * 2), (int16_t)((y1) * 2), (int16_t)((x2) * 2), (int16_t)((y2) * 2), 0 }
#define C(x, y, r) { RING, (int16_t)((x) * 2), (int16_t)((y) * 2), (int16_t)((r) * 2), 0, 0 }
#define A(x, y, r, a0, a1) { ARC, (int16_t)((x) * 2), (int16_t)((y) * 2), (int16_t)((r) * 2), a0, a1 }
#define D(x, y, r) { DISC, (int16_t)((x) * 2), (int16_t)((y) * 2), (int16_t)((r) * 2), 0, 0 }
#define P(x, y, r, a0, a1) { PIE, (int16_t)((x) * 2), (int16_t)((y) * 2), (int16_t)((r) * 2), a0, a1 }
#define R(x, y, w, h) L(x, y, (x) + (w), y), L((x) + (w), y, (x) + (w), (y) + (h)), L((x) + (w), (y) + (h), x, (y) + (h)), L(x, (y) + (h), x, y)
#define Z { END, 0, 0, 0, 0, 0 }

const Prim iBack[] = { L(15, 5, 8, 12), L(8, 12, 15, 19), Z };
const Prim iPower[] = { L(12, 3, 12, 11), A(12, 13, 8, 305, 595), Z };
const Prim iCheck[] = { L(5, 12.5, 10, 17.5), L(10, 17.5, 19, 7), Z };
const Prim iUp[] = { L(6, 15, 12, 9), L(12, 9, 18, 15), Z };
const Prim iDown[] = { L(6, 9, 12, 15), L(12, 15, 18, 9), Z };
const Prim iLock[] = { R(5, 11, 14, 10), L(8, 11, 8, 8), L(16, 11, 16, 8), A(12, 8, 4, 180, 360), Z };
const Prim iOutput[] = { R(3, 5, 18, 14), L(7, 12, 11, 12), L(9, 10, 9, 14), L(14, 12, 17, 12), Z };
const Prim iBattery[] = { R(2, 7, 18, 10), L(22, 11, 22, 13), L(5, 10, 5, 14), L(8, 10, 8, 14), L(11, 10, 11, 14), Z };
const Prim iCells[] = { L(5, 20, 5, 9), L(10, 20, 10, 6), L(15, 20, 15, 11), L(20, 20, 20, 8), Z };
const Prim iUsb[] = { L(12, 3, 12, 17), C(12, 19, 2), L(12, 13, 8, 10), L(8, 10, 8, 7), L(12, 10, 16, 7), L(16, 7, 16, 5), Z };
const Prim iPath[] = { L(3, 12, 7, 12), L(7, 12, 10, 5), L(10, 5, 14, 19), L(14, 19, 17, 12), L(17, 12, 21, 12), Z };
const Prim iTemp[] = { L(10, 14, 10, 5), L(14, 14, 14, 5), A(12, 5, 2, 180, 360), A(12, 17, 4, 300, 600), Z };
const Prim iShield[] = { L(12, 3, 20, 6), L(20, 6, 20, 12), L(20, 12, 18.5, 16.5), L(18.5, 16.5, 15.5, 19.5), L(15.5, 19.5, 12, 21),
                         L(12, 21, 8.5, 19.5), L(8.5, 19.5, 5.5, 16.5), L(5.5, 16.5, 4, 12), L(4, 12, 4, 6), L(4, 6, 12, 3), Z };
const Prim iList[] = { L(4, 6, 20, 6), L(4, 12, 20, 12), L(4, 18, 14, 18), Z };
const Prim iSeq[] = { L(3, 18, 8, 18), L(8, 18, 8, 12), L(8, 12, 13, 12), L(13, 12, 13, 6), L(13, 6, 21, 6), Z };
const Prim iBolt[] = { L(13, 2, 4, 14), L(4, 14, 11, 14), L(11, 14, 10, 22), L(10, 22, 19, 10), L(19, 10, 12, 10), L(12, 10, 13, 2), Z };
const Prim iDiag[] = { L(3, 12, 7, 12), L(7, 12, 9, 7), L(9, 7, 13, 17), L(13, 17, 15, 12), L(15, 12, 21, 12), Z };
const Prim iNet[] = { R(9, 3, 6, 5), R(3, 16, 6, 5), R(15, 16, 6, 5), L(12, 8, 12, 12), L(6, 16, 6, 12), L(6, 12, 18, 12), L(18, 12, 18, 16), Z };
const Prim iTheme[] = { C(12, 12, 9), P(12, 12, 9, 90, 270), Z };
const Prim iService[] = { L(14, 7, 17, 4), L(17, 4, 20, 7), L(17, 4, 17, 12), A(12, 12, 5, 0, 90), L(12, 17, 4, 17), Z };
const Prim iFan[] = { C(12, 12, 2), A(12, 7, 5, 200, 340), A(17, 12, 5, 290, 430), A(12, 17, 5, 20, 160), A(7, 12, 5, 110, 250), Z };
const Prim iMonitor[] = { R(3, 4, 18, 13), L(8, 21, 16, 21), L(12, 17, 12, 21), L(7, 9, 10, 12), L(10, 12, 7, 15), L(12, 15, 16, 15), Z };
const Prim iPlus[] = { L(12, 5, 12, 19), L(5, 12, 19, 12), Z };
const Prim iMinus[] = { L(5, 12, 19, 12), Z };
const Prim iDel[] = { L(9, 6, 21, 6), L(21, 6, 21, 18), L(21, 18, 9, 18), L(9, 18, 3, 12), L(3, 12, 9, 6), L(12, 9, 17, 15), L(17, 9, 12, 15), Z };
const Prim iDot[] = { D(12, 12, 4), Z };

const Prim* shape(Icon icon)
{
    switch (icon) {
    case ICON_BACK: return iBack;
    case ICON_POWER: return iPower;
    case ICON_CHECK: return iCheck;
    case ICON_UP: return iUp;
    case ICON_DOWN: return iDown;
    case ICON_LOCK: return iLock;
    case ICON_OUTPUT: return iOutput;
    case ICON_BATTERY: return iBattery;
    case ICON_CELLS: return iCells;
    case ICON_USB: return iUsb;
    case ICON_PATH: return iPath;
    case ICON_TEMP: return iTemp;
    case ICON_SHIELD: return iShield;
    case ICON_LIST: return iList;
    case ICON_SEQ: return iSeq;
    case ICON_BOLT: return iBolt;
    case ICON_DIAG: return iDiag;
    case ICON_NET: return iNet;
    case ICON_THEME: return iTheme;
    case ICON_SERVICE: return iService;
    case ICON_FAN: return iFan;
    case ICON_MONITOR: return iMonitor;
    case ICON_PLUS: return iPlus;
    case ICON_MINUS: return iMinus;
    case ICON_DEL: return iDel;
    case ICON_DOT: return iDot;
    default: return 0;
    }
}

float segment(float px, float py, float x1, float y1, float x2, float y2)
{
    const float dx = x2 - x1, dy = y2 - y1;
    const float len = dx * dx + dy * dy;
    float t = len > 0 ? ((px - x1) * dx + (py - y1) * dy) / len : 0;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    const float ex = x1 + t * dx - px, ey = y1 + t * dy - py;
    return sqrtf(ex * ex + ey * ey);
}

bool inAngle(float px, float py, float cx, float cy, int a0, int a1)
{
    float deg = atan2f(py - cy, px - cx) * 57.29578f;
    if (deg < 0) deg += 360;
    while (deg < a0) deg += 360;
    return deg <= a1;
}

// Coverage 0..1 of a pixel centre (grid units) by the icon.
float coverage(const Prim* p, float x, float y, float halfWidth, float pixel)
{
    float best = 0;
    for (; p->type != END; ++p) {
        const float a = p->a * 0.5f, b = p->b * 0.5f, c = p->c * 0.5f, d = p->d * 0.5f;
        float dist = 1e9f, cov = 0;
        switch (p->type) {
        case LINE: dist = segment(x, y, a, b, c, d); break;
        case RING: dist = fabsf(sqrtf((x - a) * (x - a) + (y - b) * (y - b)) - c); break;
        case ARC:
            if (inAngle(x, y, a, b, p->d, p->e)) dist = fabsf(sqrtf((x - a) * (x - a) + (y - b) * (y - b)) - c);
            else {
                const float s0 = p->d / 57.29578f, s1 = p->e / 57.29578f;
                const float d0 = sqrtf((x - a - c * cosf(s0)) * (x - a - c * cosf(s0)) + (y - b - c * sinf(s0)) * (y - b - c * sinf(s0)));
                const float d1 = sqrtf((x - a - c * cosf(s1)) * (x - a - c * cosf(s1)) + (y - b - c * sinf(s1)) * (y - b - c * sinf(s1)));
                dist = d0 < d1 ? d0 : d1;
            }
            break;
        case DISC: {
            const float r = sqrtf((x - a) * (x - a) + (y - b) * (y - b));
            cov = (c - r) / pixel + 0.5f;
            break;
        }
        case PIE: {
            const float r = sqrtf((x - a) * (x - a) + (y - b) * (y - b));
            if (inAngle(x, y, a, b, p->d, p->e)) cov = (c - r) / pixel + 0.5f;
            break;
        }
        }
        if (p->type == LINE || p->type == RING || p->type == ARC) cov = (halfWidth - dist) / pixel + 0.5f;
        if (cov > best) best = cov;
        if (best >= 1) return 1;
    }
    return best < 0 ? 0 : best;
}
}

namespace {
struct IconCacheEntry {
    uint8_t icon, size;
    colortype ink, base;
    touchgfx::BitmapId id;
    uint16_t* pixels;
};
IconCacheEntry iconCache[64];
#ifndef SIMULATOR
// Textures live next to the frame buffers (external RAM), not in AXI SRAM.
LOCATION_PRAGMA_NOLOAD("TouchGFX_Framebuffer")
uint16_t iconPixels[64][24 * 24] __attribute__((aligned(32))) LOCATION_ATTRIBUTE_NOLOAD("TouchGFX_Framebuffer");
#endif
unsigned iconCacheCount = 0;

colortype blend(colortype base, colortype ink, float a)
{
    using touchgfx::Color;
    const float b = 1 - a;
    return Color::getColorFromRGB((uint8_t)(Color::getRed(base) * b + Color::getRed(ink) * a + 0.5f),
                                  (uint8_t)(Color::getGreen(base) * b + Color::getGreen(ink) * a + 0.5f),
                                  (uint8_t)(Color::getBlue(base) * b + Color::getBlue(ink) * a + 0.5f));
}
}

void drawIcon(Icon icon, const Rect& box, const Rect& clip, colortype ink, colortype base, int size)
{
    const Prim* p = shape(icon);
    if (!p) return;
    if (size > 24) size = 24;
    const int x0 = box.x + (box.width - size) / 2, y0 = box.y + (box.height - size) / 2;
    const Rect area = Rect(x0, y0, size, size) & clip;
    if (area.isEmpty()) return;
    const float unit = 24.0f / size; // grid units per pixel
    const float halfWidth = 1.0f;    // 2 unit stroke like the mockup
#ifndef SIMULATOR
    // Target: render once into an RGB565 texture composited over the known backdrop.
    IconCacheEntry* entry = 0;
    for (unsigned n = 0; n < iconCacheCount; ++n) {
        IconCacheEntry& c = iconCache[n];
        if (c.icon == icon && c.size == size && c.ink == ink && c.base == base) { entry = &c; break; }
    }
    if (!entry && iconCacheCount < sizeof(iconCache) / sizeof(iconCache[0])) {
        IconCacheEntry& c = iconCache[iconCacheCount];
        c.pixels = iconPixels[iconCacheCount];
        c.icon = (uint8_t)icon; c.size = (uint8_t)size; c.ink = ink; c.base = base;
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x) {
                float a = coverage(p, (x + 0.5f) * unit, (y + 0.5f) * unit, halfWidth, unit);
                if (a > 1) a = 1;
                const colortype k = blend(base, ink, a);
                c.pixels[y * size + x] = ((touchgfx::Color::getRed(k) & 0xF8) << 8) | ((touchgfx::Color::getGreen(k) & 0xFC) << 3) | (touchgfx::Color::getBlue(k) >> 3);
            }
        SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(c.pixels), sizeof(iconPixels[0]));
        c.id = touchgfx::Bitmap::dynamicBitmapCreateExternal(size, size, c.pixels, touchgfx::Bitmap::RGB565);
        if (c.id != touchgfx::BITMAP_INVALID) { ++iconCacheCount; entry = &c; }
    }
    if (entry) {
        Rect local = area;
        local.x -= x0; local.y -= y0;
        touchgfx::HAL::lcd().drawPartialBitmap(touchgfx::Bitmap(entry->id), x0, y0, local);
        return;
    }
#else
    (void)base;
#endif
    for (int y = area.y; y < area.bottom(); ++y)
        for (int x = area.x; x < area.right();) {
            float c = coverage(p, (x - x0 + 0.5f) * unit, (y - y0 + 0.5f) * unit, halfWidth, unit);
            if (c > 1) c = 1;
            const uint8_t alpha = (uint8_t)(c * 255);
            int end = x + 1;
            if (alpha == 255 || alpha < 6)
                while (end < area.right()) {
                    float n = coverage(p, (end - x0 + 0.5f) * unit, (y - y0 + 0.5f) * unit, halfWidth, unit);
                    if (n > 1) n = 1;
                    if ((uint8_t)(n * 255) != alpha && !(alpha < 6 && n * 255 < 6)) break;
                    ++end;
                }
            if (alpha >= 6) ui::fillRect(Rect(x, y, end - x, 1), clip, ink, alpha);
            x = end;
        }
}
}
