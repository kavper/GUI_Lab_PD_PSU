import json
from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
touchgfx_path = root / 'Appli/TouchGFX/GUI_Lab_PD_PSU.touchgfx'
texts_path = root / 'Appli/TouchGFX/assets/texts/texts.xml'

with touchgfx_path.open(encoding='utf-8-sig') as handle:
    project = json.load(handle)
screens = project['Application']['Screens']

def text(name, text_id, x, y, w, h, color=(238, 244, 251)):
    return {'Type':'TextArea','Name':name,'X':x,'Y':y,'Width':w,'Height':h,
            'TextId':text_id,'TextRotation':'0',
            'Color':{'Red':color[0],'Green':color[1],'Blue':color[2]}}

def button(name, text_id, x, y, w=350, h=78, small=False):
    return {'Type':'ButtonWithLabel','Name':name,'X':x,'Y':y,'Width':w,'Height':h,
            'Pressed':'btn_menu_pressed_350x78.png' if not small else 'btn_output_v2_on_150x44.png',
            'Released':'btn_menu_released_350x78.png' if not small else 'btn_output_v2_off_150x44.png',
            'TextId':text_id,
            'ReleasedColor':{'Red':238,'Green':244,'Blue':251},
            'PressedColor':{'Red':5,'Green':19,'Blue':26},'TextRotation':'0'}

def goto(trigger, destination, name):
    return {'InteractionName':name,'Trigger':{'Type':'TriggerClicked','TriggerComponent':trigger},
            'Action':{'Type':'ActionGotoScreen','ScreenTransitionType':'ScreenTransitionWipe',
                      'ScreenTransitionDirection':'West','ActionComponent':destination}}

def back():
    return {'InteractionName':'BackToSettings','Trigger':{'Type':'TriggerClicked','TriggerComponent':'BackButton'},
            'Action':{'Type':'ActionGotoScreen','ScreenTransitionType':'ScreenTransitionWipe',
                      'ScreenTransitionDirection':'East','ActionComponent':'ScreenSettings'}}

settings = next(s for s in screens if s['Name'] == 'ScreenSettings')
settings['Components'] = [
    {'Type':'Image','Name':'SettingsBackground','Width':800,'Height':480,'RelativeFilename':'bg_settings_800x480.png'},
    text('SettingsTitle','TXT_CONTROL_CENTER',28,17,330,36),
    button('BackButton','TXT_SETTINGS_BACK',628,10,150,44,True),
]
entries = [
    ('PresetManagerButton','TXT_MENU_PRESETS','ScreenPresets'),
    ('SequencerButton','TXT_MENU_SEQUENCER','ScreenSequencer'),
    ('UsbPdButton','TXT_MENU_USB_PD','ScreenUsbPd'),
    ('BmsButton','TXT_MENU_BMS','ScreenBms'),
    ('MeasurementsButton','TXT_MENU_MEASUREMENTS','ScreenMeasurements'),
    ('DiagnosticsButton','TXT_MENU_DIAGNOSTICS','ScreenDiagnostics'),
    ('OutputLimitsButton','TXT_MENU_LIMITS','ScreenLimits'),
    ('SystemButton','TXT_MENU_SYSTEM','ScreenSystem'),
]
for index, (name, tid, _) in enumerate(entries):
    settings['Components'].append(button(name, tid, 25 + (index % 2) * 400, 76 + (index // 2) * 91))
settings['Interactions'] = [goto(name, dest, 'Open' + dest[6:]) for name, _, dest in entries]
settings['Interactions'].append({
    'InteractionName':'CloseSettings','Trigger':{'Type':'TriggerClicked','TriggerComponent':'BackButton'},
    'Action':{'Type':'ActionGotoScreen','ScreenTransitionType':'ScreenTransitionWipe',
              'ScreenTransitionDirection':'East','ActionComponent':'Screen1'}})

details = [
    ('ScreenPresets','TXT_PRESET_MANAGER_TITLE','TXT_PRESET_MANAGER_BODY'),
    ('ScreenSequencer','TXT_SEQUENCER_TITLE','TXT_SEQUENCER_BODY'),
    ('ScreenUsbPd','TXT_USB_PD_TITLE','TXT_USB_PD_BODY'),
    ('ScreenBms','TXT_BMS_TITLE','TXT_BMS_BODY'),
    ('ScreenMeasurements','TXT_MEASUREMENTS_TITLE','TXT_MEASUREMENTS_BODY'),
    ('ScreenLimits','TXT_LIMITS_TITLE','TXT_LIMITS_BODY'),
    ('ScreenSystem','TXT_SYSTEM_TITLE','TXT_SYSTEM_BODY'),
]
screens[:] = [s for s in screens if s['Name'] not in {d[0] for d in details}]
for screen_name, title_id, body_id in details:
    screens.append({'Name':screen_name,'Components':[
        {'Type':'Image','Name':'Background','Width':800,'Height':480,'RelativeFilename':'bg_settings_800x480.png'},
        text('Title',title_id,28,17,500,36),
        button('BackButton','TXT_SETTINGS_BACK',628,10,150,44,True),
        text('Content',body_id,42,88,716,330,(202,219,234)),
        text('Footer','TXT_MODULE_PREVIEW',42,430,716,22,(42,199,217)),
    ],'Interactions':[back()]})

# OUTPUT is status-only. Keep the generated member for application code, but
# hide the toggle and remove its click interaction so the screen cannot enable power.
main = next(s for s in screens if s['Name'] == 'Screen1')
for component in main['Components']:
    if component.get('Name') == 'OutputEnable':
        component['Visible'] = False
main['Interactions'] = [i for i in main.get('Interactions', [])
                        if i.get('Trigger', {}).get('TriggerComponent') != 'OutputEnable']

with touchgfx_path.open('w', encoding='utf-8') as handle:
    json.dump(project, handle, indent=2, ensure_ascii=False)
    handle.write('\n')

tree = ET.parse(texts_path)
group = tree.find('./Texts/TextGroup')
existing = {node.attrib['Id'] for node in group.findall('Text')}
new_texts = {
    'TXT_CONTROL_CENTER':('Left','SettingsTitle','CONTROL CENTER'),
    'TXT_MENU_SEQUENCER':('Center','MenuTitle','SEQUENCER'),
    'TXT_MENU_USB_PD':('Center','MenuTitle','USB POWER DELIVERY'),
    'TXT_MENU_BMS':('Center','MenuTitle','BATTERY / BMS'),
    'TXT_MENU_MEASUREMENTS':('Center','MenuTitle','MEASUREMENTS'),
    'TXT_MODULE_PREVIEW':('Left','MenuHint','UI READY - CONTROLLER COMMANDS WILL BE CONNECTED IN THE NEXT PROTOCOL REVISION'),
    'TXT_PRESET_MANAGER_TITLE':('Left','SettingsTitle','PRESET MANAGER'),
    'TXT_PRESET_MANAGER_BODY':('Left','Button','PRESET 1   5.000 V / 1.000 A\nPRESET 2  12.00 V / 2.000 A\nPRESET 3  20.00 V / 3.000 A\n\nEDIT NAME, VOLTAGE AND CURRENT LIMIT\nSAVE TO NONVOLATILE STORAGE\nSTARTUP PRESET AND RECALL BEHAVIOR'),
    'TXT_SEQUENCER_TITLE':('Left','SettingsTitle','OUTPUT SEQUENCER'),
    'TXT_SEQUENCER_BODY':('Left','Button','STEP  V SET   I LIMIT  DURATION  SLEW\n01    5.000 V 1.000 A  2.0 S     1.0 V/S\n02   12.00 V  2.000 A  5.0 S     0.5 V/S\n03    0.000 V 0.000 A  1.0 S     FAST\n\nLOOPS  1     STATE  READY\nRUN / PAUSE / ABORT WITH LIVE TIMELINE'),
    'TXT_USB_PD_TITLE':('Left','SettingsTitle','USB POWER DELIVERY'),
    'TXT_USB_PD_BODY':('Left','Button','ROLE        AUTO / DRP\nPOLICY      SINK + SOURCE\nCONTRACT    WAITING FOR PD CONTROLLER\nVBUS        0.000 V\nCURRENT     0.000 A\nPOWER       0.00 W\n\nPDO / PPS PROFILE SELECTION, CABLE DATA AND PD FAULTS'),
    'TXT_BMS_TITLE':('Left','SettingsTitle','BATTERY MANAGEMENT'),
    'TXT_BMS_BODY':('Left','Button','PACK        WAITING FOR BMS TELEMETRY\nSOC / SOH   ---% / ---%\nCURRENT     0.000 A\nPOWER       0.00 W\nCOULOMB     0.000 AH\nCYCLES      ---\n\nCELL VOLTAGES, TEMPERATURES, BALANCING AND FAULT STATUS'),
    'TXT_MEASUREMENTS_TITLE':('Left','SettingsTitle','MEASUREMENTS'),
    'TXT_MEASUREMENTS_BODY':('Left','Button','OUTPUT      VOLTAGE / CURRENT / POWER\nINPUT       VIN / CURRENT / POWER\nTHERMAL     MOSFET / PCB / AMBIENT\nENERGY      WH / AH / ELAPSED TIME\n\nLIVE GRAPH, MIN / MAX / AVERAGE AND EXPORT SNAPSHOT'),
    'TXT_LIMITS_TITLE':('Left','SettingsTitle','PROTECTION LIMITS'),
    'TXT_LIMITS_BODY':('Left','Button','MAX VOLTAGE       27.00 V\nMAX CURRENT        5.000 A\nMAX POWER          --- W\nOVP / OCP / OTP    ENABLED\n\nSOFT START, TRIP DELAY, FAULT LATCH AND SAFE STARTUP STATE'),
    'TXT_SYSTEM_TITLE':('Left','SettingsTitle','DISPLAY & SYSTEM'),
    'TXT_SYSTEM_BODY':('Left','Button','THEME       DARK\nBRIGHTNESS  80%\nDIM TIME    60 S\nFIRMWARE    v0.1.0\n\nLANGUAGE, TOUCH CALIBRATION, ABOUT, UPDATE AND FACTORY RESET'),
}
for text_id, (align, typography, value) in new_texts.items():
    if text_id in existing:
        continue
    node = ET.SubElement(group, 'Text', {'Id':text_id,'Alignment':align,'TypographyId':typography})
    translation = ET.SubElement(node, 'Translation', {'Language':'GB'})
    translation.text = value
ET.indent(tree, space='  ')
tree.write(texts_path, encoding='utf-8', xml_declaration=True)
