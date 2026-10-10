#include <gui/common/UiTheme.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Bitmap.hpp>
#include <gui/screenpresets_screen/ScreenPresetsView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
}
ScreenPresetsView::ScreenPresetsView():selected(0),field(0) {psu_editor_clear(&editor);}
void ScreenPresetsView::setupScreen(){ScreenPresetsViewBase::setupScreen();FieldV.setHeight(80);FieldI.setHeight(80);lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,"Three editable output presets",lab_muted());select(0);
    setupTheme();
}
void ScreenPresetsView::select(unsigned i){selected=i;const PsuPreset* p=psu_preset_get(i);psu_editor_load_milli(&editor,p?(field?p->current_ma:p->voltage_mv):0,3);refresh();}
void ScreenPresetsView::refresh(){
 PresetCard1.setBitmaps(touchgfx::Bitmap(selected==0?BITMAP_UX_TAB_SEL_160X58_ID:BITMAP_UX_TAB_REL_160X58_ID),touchgfx::Bitmap(BITMAP_UX_TAB_SEL_160X58_ID));PresetCard1.invalidate();
 PresetCard2.setBitmaps(touchgfx::Bitmap(selected==1?BITMAP_UX_TAB_SEL_160X58_ID:BITMAP_UX_TAB_REL_160X58_ID),touchgfx::Bitmap(BITMAP_UX_TAB_SEL_160X58_ID));PresetCard2.invalidate();
 PresetCard3.setBitmaps(touchgfx::Bitmap(selected==2?BITMAP_UX_TAB_SEL_160X58_ID:BITMAP_UX_TAB_REL_160X58_ID),touchgfx::Bitmap(BITMAP_UX_TAB_SEL_160X58_ID));PresetCard3.invalidate();
 char b[64];const PsuPreset* p=psu_preset_get(selected);
 snprintf(b,sizeof(b),"PRESET %u / %s",selected+1,field?"CURRENT":"VOLTAGE");lab_show(EditHeading,EditHeadingBuffer,EDITHEADING_SIZE,b,lab_cyan());
 snprintf(b,sizeof(b),"%lu.%03lu",(unsigned long)(p->voltage_mv/1000),(unsigned long)(p->voltage_mv%1000));lab_show(ValueV,ValueVBuffer,VALUEV_SIZE,field?b:editor.text,field?lab_text():lab_cyan());
 snprintf(b,sizeof(b),"%lu.%03lu",(unsigned long)(p->current_ma/1000),(unsigned long)(p->current_ma%1000));lab_show(ValueI,ValueIBuffer,VALUEI_SIZE,field?editor.text:b,field?lab_cyan():lab_text());
 LabelV.setColor(field?lab_muted():lab_cyan());LabelV.invalidate();LabelI.setColor(field?lab_cyan():lab_muted());LabelI.invalidate();
 p=psu_preset_get(0);snprintf(b,sizeof(b),"%lu.%03lu V\n%lu.%03lu A",(unsigned long)(p->voltage_mv/1000),(unsigned long)(p->voltage_mv%1000),(unsigned long)(p->current_ma/1000),(unsigned long)(p->current_ma%1000));lab_show(Summary1,Summary1Buffer,SUMMARY1_SIZE,b,selected==0?lab_cyan():lab_muted());
 p=psu_preset_get(1);snprintf(b,sizeof(b),"%lu.%03lu V\n%lu.%03lu A",(unsigned long)(p->voltage_mv/1000),(unsigned long)(p->voltage_mv%1000),(unsigned long)(p->current_ma/1000),(unsigned long)(p->current_ma%1000));lab_show(Summary2,Summary2Buffer,SUMMARY2_SIZE,b,selected==1?lab_cyan():lab_muted());
 p=psu_preset_get(2);snprintf(b,sizeof(b),"%lu.%03lu V\n%lu.%03lu A",(unsigned long)(p->voltage_mv/1000),(unsigned long)(p->voltage_mv%1000),(unsigned long)(p->current_ma/1000),(unsigned long)(p->current_ma%1000));lab_show(Summary3,Summary3Buffer,SUMMARY3_SIZE,b,selected==2?lab_cyan():lab_muted());
}
void ScreenPresetsView::key(char c){psu_editor_key(&editor,c);refresh();}
void ScreenPresetsView::allOff(){psu_app_power_shutdown();}
void ScreenPresetsView::editVoltage(){field=0;select(selected);}
void ScreenPresetsView::editCurrent(){field=1;select(selected);}
void ScreenPresetsView::loadPreset(){lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,psu_preset_apply(selected)?"Preset requested; output state unchanged":"Preset request blocked",lab_cyan());}
void ScreenPresetsView::presetKeyClr(){psu_editor_clear(&editor);refresh();}
void ScreenPresetsView::presetKeyDel(){psu_editor_backspace(&editor);refresh();}
void ScreenPresetsView::presetKeyApply(){uint32_t v;if(!psu_editor_parse_milli(&editor,&v)){psu_editor_clear(&editor);refresh();return;}uint32_t max=field?5000:27000;bool clamped=v>max;if(clamped)v=max;const PsuPreset* p=psu_preset_get(selected);psu_preset_set(selected,field?p->voltage_mv:v,field?v:p->current_ma);select(selected);lab_show(PageFeedback,PageFeedbackBuffer,PAGEFEEDBACK_SIZE,clamped?"Saved at the maximum allowed value":"Preset saved",lab_cyan());}
void ScreenPresetsView::selectPreset1(){select(0);}
void ScreenPresetsView::selectPreset2(){select(1);}
void ScreenPresetsView::selectPreset3(){select(2);}
void ScreenPresetsView::presetKey0(){key('0');}
void ScreenPresetsView::presetKey1(){key('1');}
void ScreenPresetsView::presetKey2(){key('2');}
void ScreenPresetsView::presetKey3(){key('3');}
void ScreenPresetsView::presetKey4(){key('4');}
void ScreenPresetsView::presetKey5(){key('5');}
void ScreenPresetsView::presetKey6(){key('6');}
void ScreenPresetsView::presetKey7(){key('7');}
void ScreenPresetsView::presetKey8(){key('8');}
void ScreenPresetsView::presetKey9(){key('9');}
void ScreenPresetsView::presetKeyDot(){key('.');}

void ScreenPresetsView::setupTheme()
{
    ui::ThemeScreen& theme=ui::ThemeScreen::get();
    theme.begin(*this);
    theme.box(LabBackground,ui::BACKGROUND);
    theme.box(LabHeader,ui::SURFACE);
    theme.box(ThemeHeaderDivider,ui::BORDER);
    theme.text(PageTitle);
    theme.text(PageFeedback);
    theme.button(AllOffButton,ui::DANGER);
    theme.button(BackButton,ui::NORMAL);
    theme.button(PresetKey1,ui::NORMAL);
    theme.button(PresetKey2,ui::NORMAL);
    theme.button(PresetKey3,ui::NORMAL);
    theme.button(PresetKey4,ui::NORMAL);
    theme.button(PresetKey5,ui::NORMAL);
    theme.button(PresetKey6,ui::NORMAL);
    theme.button(PresetKey7,ui::NORMAL);
    theme.button(PresetKey8,ui::NORMAL);
    theme.button(PresetKey9,ui::NORMAL);
    theme.button(PresetKeyClr,ui::NORMAL);
    theme.button(PresetKey0,ui::NORMAL);
    theme.button(PresetKeyDot,ui::NORMAL);
    theme.button(PresetKeyDel,ui::NORMAL);
    theme.button(PresetKeyApply,ui::PRIMARY);
    theme.text(EditHeading);
    theme.button(PresetCard1,ui::NORMAL);
    theme.text(Summary1);
    theme.button(PresetCard2,ui::NORMAL);
    theme.text(Summary2);
    theme.button(PresetCard3,ui::NORMAL);
    theme.text(Summary3);
    theme.panel(FieldBackgroundV);
    theme.text(LabelV);
    theme.text(ValueV);
    theme.panel(FieldBackgroundI);
    theme.text(LabelI);
    theme.text(ValueI);
    theme.button(LoadButton,ui::NORMAL);
    theme.text(SaveHint);
    theme.apply();
}
