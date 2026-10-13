#include <gui/common/UiTheme.hpp>
#include <gui/common/RoundedCorners.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/Font.hpp>
#include <images/BitmapDatabase.hpp>
#include <assert.h>
#ifndef SIMULATOR
#include "main.h"
#endif
namespace ui {
namespace {
bool darkMode=true;
unsigned accent=0;
const uint32_t lightAccents[]={0x2457E6,0x007F73,0x743FD3,0xAB5D07};
const uint32_t darkAccents[]={0x5793FF,0x2DD4BF,0xA78BFA,0xFBBF24};
touchgfx::colortype rgb(uint32_t value){return touchgfx::Color::getColorFromRGB(value>>16,(value>>8)&255,value&255);}
uint32_t raw(Role role,bool dark,unsigned index) {
    const uint32_t normal[]={0xF5F7FC,0xFFFFFF,0xEFF3FA,0xDDE3EB,0x172337,0x607085};
    const uint32_t night[]={0x0F172A,0x162235,0x1E2D44,0x314158,0xE8EEF8,0xA7B6CB};
    if(role<=MUTED)return dark?night[role]:normal[role];
    if(role==ACCENT)return dark?darkAccents[index]:lightAccents[index];
    if(role==ACCENT_SOFT){
        uint32_t base=dark?night[SURFACE]:normal[SURFACE],a=dark?darkAccents[index]:lightAccents[index];
        uint32_t result=0;for(unsigned shift=0;shift<24;shift+=8)result|=((((base>>shift)&255)*9+((a>>shift)&255))/10)<<shift;
        return result;
    }
    if(role==POSITIVE)return dark?0x4ADE80:0x167548;
    if(role==CAUTION)return dark?0xF4B85E:0xA85B05;
    if(role==NEGATIVE)return dark?0xF88A93:0xB43E45;
    return dark?0x0F172A:0xFFFFFF;
}
bool selectedBitmap(touchgfx::BitmapId id){
    switch(id) {
    // Kept as bitmap IDs so existing tab/chemistry/selection logic still works.
    case BITMAP_UX_TAB_SEL_160X58_ID: case BITMAP_UX_TAB_SEL_160X38_ID:
    case BITMAP_UX_ACTION_SEL_88X48_ID: case BITMAP_UX_ROW_SEL_432X36_ID:
    case BITMAP_BATTERY_TAB_SEL_180X42_ID: case BITMAP_MENU_TILE_SEL_248X92_ID: case BITMAP_UX_TILE_SEL_248X112_ID: case BITMAP_MAIN_PRESET_SEL_154X60_ID:
    case BITMAP_MAIN_PRESET_SEL_100X60_ID:case BITMAP_MAIN_SENSE_SEL_148X60_ID:return true;
    default:return false;
    }
}
void fill(touchgfx::Rect rect,const touchgfx::Rect& clip,touchgfx::colortype color,uint8_t alpha=255){rect&=clip;if(!rect.isEmpty())touchgfx::HAL::lcd().fillRect(rect,color,alpha);}
touchgfx::colortype composite(touchgfx::colortype base,touchgfx::colortype border,touchgfx::colortype surface,unsigned outer,unsigned inner){
    const unsigned b=outer-inner,a=64-outer;
    return touchgfx::Color::getColorFromRGB(
        (touchgfx::Color::getRed(base)*a+touchgfx::Color::getRed(border)*b+touchgfx::Color::getRed(surface)*inner+32)/64,
        (touchgfx::Color::getGreen(base)*a+touchgfx::Color::getGreen(border)*b+touchgfx::Color::getGreen(surface)*inner+32)/64,
        (touchgfx::Color::getBlue(base)*a+touchgfx::Color::getBlue(border)*b+touchgfx::Color::getBlue(surface)*inner+32)/64);
}
struct CornerCacheEntry {
    touchgfx::colortype base, border, surface;
    int radius, thickness;
    touchgfx::BitmapId ids[4];
    alignas(32) uint16_t pixels[4][12*12];
};
// Stable storage: never overwrite a texture while queued GPU commands read it.
CornerCacheEntry cornerCache[128];
unsigned cornerCacheCount=0;
bool cachedCorners(const touchgfx::Rect& rect,const touchgfx::Rect& clip,touchgfx::colortype border,touchgfx::colortype surface,touchgfx::colortype base,int radius,int thickness=1){
    CornerCacheEntry* entry=0;
    for(unsigned n=0;n<cornerCacheCount;n++) {
        auto& c=cornerCache[n];
        if(c.radius==radius && c.thickness==thickness && c.base==base && c.border==border && c.surface==surface){entry=&c;break;}
    }
    if(!entry){
        if(cornerCacheCount==128)return false;
        auto& c=cornerCache[cornerCacheCount];
        c.radius=radius;c.thickness=thickness;c.base=base;c.border=border;c.surface=surface;
        for(int y=0;y<radius;y++)for(int x=0;x<radius;x++){
            const auto color=composite(base,border,surface,ui::corners::coverage(radius,x,y),ui::corners::coverage(radius-thickness,x-thickness,y-thickness));
            const uint16_t rgb565=((touchgfx::Color::getRed(color)&0xF8)<<8)|((touchgfx::Color::getGreen(color)&0xFC)<<3)|(touchgfx::Color::getBlue(color)>>3);
            c.pixels[0][y*radius+x]=rgb565;
            c.pixels[1][y*radius+radius-1-x]=rgb565;
            c.pixels[2][(radius-1-y)*radius+x]=rgb565;
            c.pixels[3][(radius-1-y)*radius+radius-1-x]=rgb565;
        }
#ifndef SIMULATOR
        SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(c.pixels),sizeof(c.pixels));
#endif
        unsigned created=0;
        for(;created<4;created++){
            c.ids[created]=touchgfx::Bitmap::dynamicBitmapCreateExternal(radius,radius,c.pixels[created],touchgfx::Bitmap::RGB565);
            if(c.ids[created]==touchgfx::BITMAP_INVALID)break;
        }
        if(created!=4){while(created)touchgfx::Bitmap::dynamicBitmapDelete(c.ids[--created]);return false;}
        ++cornerCacheCount;entry=&c;
    }
    for(unsigned q=0;q<4;q++){
        const int x=q&1?rect.right()-radius:rect.x;
        const int y=q&2?rect.bottom()-radius:rect.y;
        touchgfx::Rect area=touchgfx::Rect(x,y,radius,radius)&clip;
        area.x-=x;area.y-=y;
        if(!area.isEmpty())touchgfx::HAL::lcd().drawPartialBitmap(touchgfx::Bitmap(entry->ids[q]),x,y,area);
    }
    return true;
}
// Identical quarter-circle masks are mirrored across both axes. Each corner
// pixel is composited once over its actual backdrop, avoiding dark fringes.
void rounded(const touchgfx::Rect& rect,const touchgfx::Rect& clip,touchgfx::colortype border,touchgfx::colortype surface,touchgfx::colortype base,int radius){
    fill(rect,clip,base);
    fill(touchgfx::Rect(rect.x,rect.y+radius,rect.width,rect.height-2*radius),clip,border);
    fill(touchgfx::Rect(rect.x+radius,rect.y,rect.width-2*radius,rect.height),clip,border);
    fill(touchgfx::Rect(rect.x+1,rect.y+radius,rect.width-2,rect.height-2*radius),clip,surface);
    fill(touchgfx::Rect(rect.x+radius,rect.y+1,rect.width-2*radius,rect.height-2),clip,surface);
    if(cachedCorners(rect,clip,border,surface,base,radius))return;
    for(int y=0;y<radius;y++)for(int x=0;x<radius;){
        const auto color=composite(base,border,surface,ui::corners::coverage(radius,x,y),ui::corners::coverage(radius-1,x-1,y-1));
        int end=x+1;
        while(end<radius && composite(base,border,surface,ui::corners::coverage(radius,end,y),ui::corners::coverage(radius-1,end-1,y-1))==color)++end;
        const int width=end-x;
        fill(touchgfx::Rect(rect.x+x,rect.y+y,width,1),clip,color);
        fill(touchgfx::Rect(rect.right()-end,rect.y+y,width,1),clip,color);
        fill(touchgfx::Rect(rect.x+x,rect.bottom()-1-y,width,1),clip,color);
        fill(touchgfx::Rect(rect.right()-end,rect.bottom()-1-y,width,1),clip,color);
        x=end;
    }
}
}
bool Theme::dark(){return darkMode;}
unsigned Theme::accentIndex(){return accent;}
void Theme::setDark(bool value){darkMode=value;ThemeScreen::get().apply();}
void Theme::setAccent(unsigned value){if(value<4){accent=value;ThemeScreen::get().apply();}}
touchgfx::colortype Theme::color(Role role){return rgb(raw(role,darkMode,accent));}
touchgfx::colortype Theme::accentColor(unsigned index){return rgb((darkMode?darkAccents:lightAccents)[index<4?index:0]);}
const char* Theme::accentName(){const char* names[]={"Blue","Teal","Violet","Amber"};return names[accent];}
touchgfx::colortype Theme::translate(touchgfx::colortype value){
    for(int role=TEXT;role<=NEGATIVE;role++) {
        if(role==ACCENT_SOFT)continue;
        for(unsigned index=0;index<4;index++)for(int mode=0;mode<2;mode++)
            if(value==rgb(raw(static_cast<Role>(role),mode!=0,index)))return color(static_cast<Role>(role));
    }
    // Older views use these equivalents for captions/status.
    if(value==rgb(0xEEF4FB)||value==rgb(0x93A3B8))return color(TEXT);
    return value;
}
ThemedSurface::ThemedSurface():original(0),button(0),label(0),image(0),style(PANEL),lastBitmap(0),lastAlpha(255){}
void ThemedSurface::bind(touchgfx::Drawable& source,SurfaceStyle kind,touchgfx::Button* input,touchgfx::ButtonWithLabel* caption,touchgfx::Image* bitmap){
    original=&source;style=kind;button=input;label=caption;image=bitmap;
    setPosition(source.getX(),source.getY(),source.getWidth(),source.getHeight());
    setTouchable(input!=0 && kind!=ICON);setVisible(source.isVisible());lastBitmap=0;lastAlpha=255;
}
bool ThemedSurface::selected() const{return button && (button->getPressedState() || selectedBitmap(button->getCurrentlyDisplayedBitmap().getId()));}
bool ThemedSurface::sync(){
    if(!original)return false;
    const uint16_t bitmap=button?button->getCurrentlyDisplayedBitmap().getId():image?image->getBitmap().getId():0;
    const uint8_t alpha=button?button->getAlpha():255;
    const bool changed=getRect()!=original->getRect() || isVisible()!=original->isVisible() || bitmap!=lastBitmap || alpha!=lastAlpha;
    if(changed){invalidate();setPosition(original->getX(),original->getY(),original->getWidth(),original->getHeight());setVisible(original->isVisible());invalidate();lastBitmap=bitmap;lastAlpha=alpha;}
    return changed;
}
touchgfx::Rect ThemedSurface::getSolidRect() const {
    if(style==SELECTION)return touchgfx::Rect();
    return touchgfx::Rect(0,0,getWidth(),getHeight());
}
void ThemedSurface::draw(const touchgfx::Rect& area) const {
    if(!original)return;
    touchgfx::Rect rect=getAbsoluteRect(),clip=area;clip.x+=rect.x;clip.y+=rect.y;
    if(style==SELECTION){
        const touchgfx::colortype accent=Theme::color(ACCENT);
        const int radius=12;
        fill(touchgfx::Rect(rect.x+radius,rect.y,rect.width-2*radius,2),clip,accent);
        fill(touchgfx::Rect(rect.x+radius,rect.bottom()-2,rect.width-2*radius,2),clip,accent);
        fill(touchgfx::Rect(rect.x,rect.y+radius,2,rect.height-2*radius),clip,accent);
        fill(touchgfx::Rect(rect.right()-2,rect.y+radius,2,rect.height-2*radius),clip,accent);
        const auto surface=Theme::color(SURFACE),base=Theme::color(BACKGROUND);
        if(!cachedCorners(rect,clip,accent,surface,base,radius,2)) {
            for(int y=0;y<radius;y++)for(int x=0;x<radius;x++) {
                const auto color=composite(base,accent,surface,ui::corners::coverage(radius,x,y),ui::corners::coverage(radius-2,x-2,y-2));
                fill(touchgfx::Rect(rect.x+x,rect.y+y,1,1),clip,color);
                fill(touchgfx::Rect(rect.right()-1-x,rect.y+y,1,1),clip,color);
                fill(touchgfx::Rect(rect.x+x,rect.bottom()-1-y,1,1),clip,color);
                fill(touchgfx::Rect(rect.right()-1-x,rect.bottom()-1-y,1,1),clip,color);
            }
        }
        // Preserve the useful divider between the measurement and setpoint.
        fill(touchgfx::Rect(rect.x+306,rect.y+12,2,rect.height-24),clip,accent);
        return;
    }
    if(style==ICON && image){
        fill(rect,clip,Theme::color(selected()?ACCENT_SOFT:SURFACE));
        const touchgfx::Bitmap bitmap=image->getBitmap();
        if(bitmap.getFormat()!=touchgfx::Bitmap::ARGB8888)return;
        const uint8_t* data=bitmap.getData();
        for(int y=0;y<rect.height;y++)for(int x=0;x<rect.width;){
            const uint8_t alpha=data[(y*bitmap.getWidth()+x)*4+3];int end=x+1;
            while(end<rect.width && data[(y*bitmap.getWidth()+end)*4+3]==alpha)++end;
            if(alpha)fill(touchgfx::Rect(rect.x+x,rect.y+y,end-x,1),clip,Theme::color(ACCENT),alpha);
            x=end;
        }
        return;
    }
    Role background=selected()?ACCENT_SOFT:SURFACE;
    touchgfx::colortype border=Theme::color(selected()?ACCENT:BORDER),ink=Theme::color(TEXT),surface=Theme::color(background);
    if(style==PRIMARY){surface=Theme::color(ACCENT);border=surface;ink=Theme::color(ON_ACCENT);}
    else if(style==DANGER){border=Theme::color(NEGATIVE);ink=border;}
    else if(style==OUTPUT && button){
        const touchgfx::BitmapId bitmapId=button->getCurrentlyDisplayedBitmap().getId();
        const bool on=bitmapId==BITMAP_BTN_OUTPUT_V2_ON_TOUCH_166X56_ID || bitmapId==BITMAP_MAIN_HEADER_BTN_SEL_140X52_ID;
        border=Theme::color(on?POSITIVE:BORDER);surface=Theme::color(on?POSITIVE:SURFACE);
    }
    else if(style==PILL){const bool cc=image && image->getBitmap().getId()==BITMAP_MODE_CC_58X36_ID;surface=Theme::color(cc?CAUTION:ACCENT);border=surface;}
    else if(style>=SWATCH_BLUE){surface=Theme::accentColor(style-SWATCH_BLUE);border=surface;ink=Theme::color(ON_ACCENT);}
    if(button && button->getAlpha()<255){surface=Theme::color(RAISED);ink=Theme::color(MUTED);}
    const int radius=style==PANEL?12:rect.height<40?6:8;
    const auto backdrop=Theme::color(ThemeScreen::get().backgroundBehind(*original));
    rounded(rect,clip,border,surface,backdrop,radius);
    if(label && label->getLabelText().hasValidId()){
        touchgfx::TextArea text;
        text.setTypedText(label->getLabelText());text.setColor(ink);
        const touchgfx::Font* font=label->getLabelText().getFont();
        const int height=font->getHeight()*font->getNumberOfLines(label->getLabelText().getText())+font->getSpacingAbove(label->getLabelText().getText());
        const int top=(rect.height-height)/2;
        text.setPosition(rect.x,rect.y+top,rect.width,height+2);
        touchgfx::Rect local=clip;local.x-=rect.x;local.y-=rect.y+top;local&=touchgfx::Rect(0,0,rect.width,height+2);
        if(!local.isEmpty())text.draw(local);
    }
    if(style>=SWATCH_BLUE && static_cast<unsigned>(style-SWATCH_BLUE)==Theme::accentIndex())
        fill(touchgfx::Rect(rect.x+8,rect.bottom()-6,rect.width-16,2),clip,ink);
}
void ThemedSurface::handleClickEvent(const touchgfx::ClickEvent& event){
    if(button && button->isTouchable()){button->handleClickEvent(event);invalidate();}
}
void ThemedSurface::handleDragEvent(const touchgfx::DragEvent& event){if(button && button->isTouchable())button->handleDragEvent(event);}
ThemeScreen::ThemeScreen():screen(0),surfaceCount(0),textCount(0),boxCount(0){}
ThemeScreen& ThemeScreen::get(){static ThemeScreen theme;return theme;}
void ThemeScreen::begin(touchgfx::Screen& value){assert(!screen);screen=&value;surfaceCount=textCount=boxCount=0;}
void ThemeScreen::detach(){if(screen)for(unsigned i=0;i<surfaceCount;i++)if(surfaces[i].getParent())static_cast<touchgfx::Container*>(surfaces[i].getParent())->remove(surfaces[i]);screen=0;surfaceCount=textCount=boxCount=0;}
void ThemeScreen::text(touchgfx::TextArea& widget,int role){assert(textCount<64);texts[textCount++]={&widget,static_cast<int8_t>(role)};}
void ThemeScreen::box(touchgfx::Box& widget,Role role){assert(boxCount<32);boxes[boxCount++]={&widget,role};}
void ThemeScreen::add(touchgfx::Drawable& widget,SurfaceStyle style,touchgfx::Button* button,touchgfx::ButtonWithLabel* label,touchgfx::Image* image){
    assert(screen && surfaceCount<48);ThemedSurface& surface=surfaces[surfaceCount++];surface.bind(widget,style,button,label,image);assert(widget.getParent());static_cast<touchgfx::Container*>(widget.getParent())->insert(&widget,surface);
}
void ThemeScreen::panel(touchgfx::Box& widget){add(widget,PANEL,0,0,0);}
void ThemeScreen::button(touchgfx::Button& widget,SurfaceStyle style){add(widget,style,&widget,0,0);}
void ThemeScreen::button(touchgfx::ButtonWithLabel& widget,SurfaceStyle style){add(widget,style,&widget,&widget,0);}
void ThemeScreen::image(touchgfx::Image& widget,SurfaceStyle style,touchgfx::Button* tile){widget.setAlpha(0);add(widget,style,tile,0,&widget);}
void ThemeScreen::sync(){
    if(!screen)return;
    for(unsigned i=0;i<textCount;i++){auto& b=texts[i];const auto color=b.role<0?Theme::translate(b.widget->getColor()):Theme::color(static_cast<Role>(b.role));if(b.widget->getColor()!=color){b.widget->setColor(color);b.widget->invalidate();}}
    for(unsigned i=0;i<boxCount;i++){auto& b=boxes[i];const auto color=static_cast<int>(b.role)<0 ? Theme::translate(b.widget->getColor()):Theme::color(b.role);if(b.widget->getColor()!=color){b.widget->setColor(color);b.widget->invalidate();}}
    for(unsigned i=0;i<surfaceCount;i++)surfaces[i].sync();
}
void ThemeScreen::apply(){sync();if(screen)screen->getRootContainer().invalidate();}
}

namespace ui {
Role ThemeScreen::backgroundBehind(const touchgfx::Drawable& widget) const {
    Role role=BACKGROUND;
    if(!screen)return role;
    const touchgfx::Rect target=widget.getAbsoluteRect();
    for(touchgfx::Drawable* child=screen->getRootContainer().getFirstChild();child && child!=&widget;child=child->getNextSibling()){
        if(!child->isVisible())continue;
        const touchgfx::Rect r=child->getAbsoluteRect();
        if(r.x>target.x||r.y>target.y||r.right()<target.right()||r.bottom()<target.bottom())continue;
        for(unsigned i=0;i<boxCount;i++)if(boxes[i].widget==child && boxes[i].role>=BACKGROUND && boxes[i].role<=RAISED)role=boxes[i].role;
        for(unsigned i=0;i<surfaceCount;i++)if(&surfaces[i]==child && surfaces[i].style==PANEL)role=SURFACE;
    }
    return role;
}
}
