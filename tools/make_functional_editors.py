import json
from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
project_path = root / 'Appli/TouchGFX/GUI_Lab_PD_PSU.touchgfx'
texts_path = root / 'Appli/TouchGFX/assets/texts/texts.xml'
project = json.loads(project_path.read_text(encoding='utf-8-sig'))
screens = project['Application']['Screens']

def text(name, tid, x, y, w, h, color=(238,244,251), wildcard=None):
    c={'Type':'TextArea','Name':name,'X':x,'Y':y,'Width':w,'Height':h,'TextId':tid,'TextRotation':'0',
       'Color':{'Red':color[0],'Green':color[1],'Blue':color[2]}}
    if wildcard:
        c['Wildcard1']={'TextId':wildcard,'UseBuffer':True,'BufferSize':256}
    return c

def small(name, tid, x, y, w=150, h=44):
    return {'Type':'ButtonWithLabel','Name':name,'X':x,'Y':y,'Width':w,'Height':h,
            'Pressed':'btn_output_v2_on_150x44.png','Released':'btn_output_v2_off_150x44.png','TextId':tid,
            'ReleasedColor':{'Red':238,'Green':244,'Blue':251},'PressedColor':{'Red':5,'Green':19,'Blue':26},'TextRotation':'0'}

def row(name, tid, x, y):
    return {'Type':'ButtonWithLabel','Name':name,'X':x,'Y':y,'Width':350,'Height':78,
            'Pressed':'btn_menu_pressed_350x78.png','Released':'btn_menu_released_350x78.png','TextId':tid,
            'ReleasedColor':{'Red':238,'Green':244,'Blue':251},'PressedColor':{'Red':5,'Green':19,'Blue':26},'TextRotation':'0'}

def custom(trigger, fn):
    return {'InteractionName':fn+'Interaction','Trigger':{'Type':'TriggerClicked','TriggerComponent':trigger},
            'Action':{'Type':'ActionCustom','FunctionName':fn}}

def back_interaction():
    return {'InteractionName':'BackToSettings','Trigger':{'Type':'TriggerClicked','TriggerComponent':'BackButton'},
            'Action':{'Type':'ActionGotoScreen','ScreenTransitionType':'ScreenTransitionWipe','ScreenTransitionDirection':'East','ActionComponent':'ScreenSettings'}}

def background(title):
    return [
        {'Type':'Image','Name':'Background','Width':800,'Height':480,'RelativeFilename':'bg_settings_800x480.png'},
        text('Title',title,28,17,500,36), small('BackButton','TXT_SETTINGS_BACK',628,10)
    ]

presets = next(s for s in screens if s['Name']=='ScreenPresets')
presets['Components'] = background('TXT_PRESET_MANAGER_TITLE') + [
    row('Preset1Button','TXT_EDITOR_PRESET1',25,76),
    row('Preset2Button','TXT_EDITOR_PRESET2',25,166),
    row('Preset3Button','TXT_EDITOR_PRESET3',25,256),
    text('EditorLabel','TXT_EDIT_SELECTED',420,82,340,24,(147,163,184)),
    text('PresetValues','TXT_EDITOR_VALUES',420,112,350,92,(42,199,217),'TXT_EDITOR_VALUES_INITIAL'),
    small('VoltageDownButton','TXT_VOLTAGE_DOWN',420,214), small('VoltageUpButton','TXT_VOLTAGE_UP',610,214),
    small('CurrentDownButton','TXT_CURRENT_DOWN',420,268), small('CurrentUpButton','TXT_CURRENT_UP',610,268),
    small('SaveButton','TXT_SAVE_PRESET',420,338), small('LoadButton','TXT_LOAD_PRESET',610,338),
    text('StatusText','TXT_EDITOR_STATUS',420,400,340,32,(54,215,137),'TXT_EDITOR_STATUS_READY'),
]
presets['Interactions']=[back_interaction(),custom('Preset1Button','selectPreset1'),custom('Preset2Button','selectPreset2'),
    custom('Preset3Button','selectPreset3'),custom('VoltageDownButton','voltageDown'),custom('VoltageUpButton','voltageUp'),
    custom('CurrentDownButton','currentDown'),custom('CurrentUpButton','currentUp'),custom('SaveButton','savePreset'),
    custom('LoadButton','loadPreset')]

seq = next(s for s in screens if s['Name']=='ScreenSequencer')
seq['Components'] = background('TXT_SEQUENCER_TITLE') + [
    row('Step1Button','TXT_STEP1',25,76), row('Step2Button','TXT_STEP2',25,166),
    row('Step3Button','TXT_STEP3',25,256), row('Step4Button','TXT_STEP4',25,346),
    text('EditorLabel','TXT_EDIT_STEP',420,78,350,24,(147,163,184)),
    text('StepValues','TXT_STEP_VALUES',420,106,350,112,(42,199,217),'TXT_STEP_VALUES_INITIAL'),
    small('VoltageDownButton','TXT_VOLTAGE_DOWN',420,224), small('VoltageUpButton','TXT_VOLTAGE_UP',610,224),
    small('CurrentDownButton','TXT_CURRENT_DOWN',420,274), small('CurrentUpButton','TXT_CURRENT_UP',610,274),
    small('TimeButton','TXT_TIME_NEXT',420,324), small('SlewButton','TXT_SLEW_NEXT',610,324),
    small('EnableButton','TXT_STEP_TOGGLE',420,380), small('RunButton','TXT_RUN_SEQUENCE',610,380),
    text('RunStatus','TXT_SEQ_STATUS',420,432,340,28,(54,215,137),'TXT_SEQ_STATUS_READY'),
]
seq['Interactions']=[back_interaction(),custom('Step1Button','selectStep1'),custom('Step2Button','selectStep2'),
    custom('Step3Button','selectStep3'),custom('Step4Button','selectStep4'),custom('VoltageDownButton','voltageDown'),
    custom('VoltageUpButton','voltageUp'),custom('CurrentDownButton','currentDown'),custom('CurrentUpButton','currentUp'),
    custom('TimeButton','nextTime'),custom('SlewButton','nextSlew'),custom('EnableButton','toggleStep'),custom('RunButton','runStop')]

project_path.write_text(json.dumps(project,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')

tree=ET.parse(texts_path); group=tree.find('./Texts/TextGroup'); existing={n.attrib['Id'] for n in group.findall('Text')}
items={
'TXT_EDITOR_PRESET1':('Center','MenuTitle','PRESET 1  -  BENCH 5 V'),
'TXT_EDITOR_PRESET2':('Center','MenuTitle','PRESET 2  -  LOGIC 12 V'),
'TXT_EDITOR_PRESET3':('Center','MenuTitle','PRESET 3  -  PD 20 V'),
'TXT_EDIT_SELECTED':('Left','Small','EDIT SELECTED PRESET'),
'TXT_EDITOR_VALUES':('Left','Button','<value>'),
'TXT_EDITOR_VALUES_INITIAL':('Left','Button','VOLTAGE  5.000 V\nCURRENT  1.000 A'),
'TXT_VOLTAGE_DOWN':('Center','Button','VOLTAGE -'),'TXT_VOLTAGE_UP':('Center','Button','VOLTAGE +'),
'TXT_CURRENT_DOWN':('Center','Button','CURRENT -'),'TXT_CURRENT_UP':('Center','Button','CURRENT +'),
'TXT_SAVE_PRESET':('Center','Button','SAVE'),'TXT_LOAD_PRESET':('Center','Button','LOAD TO MAIN'),
'TXT_EDITOR_STATUS':('Left','Small','<value>'),'TXT_EDITOR_STATUS_READY':('Left','Small','PRESET 1 SELECTED'),
'TXT_STEP1':('Center','MenuTitle','STEP 1'),'TXT_STEP2':('Center','MenuTitle','STEP 2'),
'TXT_STEP3':('Center','MenuTitle','STEP 3'),'TXT_STEP4':('Center','MenuTitle','STEP 4'),
'TXT_EDIT_STEP':('Left','Small','EDIT SELECTED STEP'),'TXT_STEP_VALUES':('Left','Button','<value>'),
'TXT_STEP_VALUES_INITIAL':('Left','Button','5.000 V  /  1.000 A\nTIME 2.0 S   SLEW 1.0 V/S\nSTEP ENABLED'),
'TXT_TIME_NEXT':('Center','Button','TIME'),'TXT_SLEW_NEXT':('Center','Button','SLEW RATE'),
'TXT_STEP_TOGGLE':('Center','Button','ENABLE / SKIP'),'TXT_RUN_SEQUENCE':('Center','Button','RUN / STOP'),
'TXT_SEQ_STATUS':('Left','Small','<value>'),'TXT_SEQ_STATUS_READY':('Left','Small','READY - 3 ENABLED STEPS'),
}
for tid,(align,typ,val) in items.items():
    node=next((n for n in group.findall('Text') if n.attrib['Id']==tid),None)
    if node is None:
        node=ET.SubElement(group,'Text',{'Id':tid,'Alignment':align,'TypographyId':typ}); tr=ET.SubElement(node,'Translation',{'Language':'GB'})
    else: tr=node.find('Translation')
    tr.text=val
ET.indent(tree,space='  '); tree.write(texts_path,encoding='utf-8',xml_declaration=True)
