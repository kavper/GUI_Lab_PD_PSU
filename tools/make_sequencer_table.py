import json
from pathlib import Path
import xml.etree.ElementTree as ET

root=Path(__file__).resolve().parents[1]
pp=root/'Appli/TouchGFX/GUI_Lab_PD_PSU.touchgfx'; tp=root/'Appli/TouchGFX/assets/texts/texts.xml'
p=json.loads(pp.read_text(encoding='utf-8-sig')); s=next(x for x in p['Application']['Screens'] if x['Name']=='ScreenSequencer')

def text(name,tid,x,y,w,h,color=(238,244,251),wild=None,size=128):
 c={'Type':'TextArea','Name':name,'X':x,'Y':y,'Width':w,'Height':h,'TextId':tid,'TextRotation':'0','Color':{'Red':color[0],'Green':color[1],'Blue':color[2]}}
 if wild:c['Wildcard1']={'TextId':wild,'UseBuffer':True,'BufferSize':size}
 return c
def button(name,tid,x,y,w=150,h=44):
 return {'Type':'ButtonWithLabel','Name':name,'X':x,'Y':y,'Width':w,'Height':h,'Pressed':'btn_output_v2_on_150x44.png','Released':'btn_output_v2_off_150x44.png','TextId':tid,'ReleasedColor':{'Red':238,'Green':244,'Blue':251},'PressedColor':{'Red':5,'Green':19,'Blue':26},'TextRotation':'0'}
def row(n,y):
 return [{'Type':'ButtonWithLabel','Name':f'Step{n}Button','X':30,'Y':y,'Width':740,'Height':56,'Pressed':'btn_sequence_row_selected_740x56.png','Released':'btn_sequence_row_released_740x56.png','TextId':'TXT_EMPTY','ReleasedColor':{'Red':238,'Green':244,'Blue':251},'PressedColor':{'Red':5,'Green':19,'Blue':26},'TextRotation':'0'},text(f'Step{n}Values','TXT_SEQ_ROW_VALUE',42,y+15,716,28,(238,244,251),f'TXT_SEQ_ROW{n}_INITIAL')]

s['Components']=[{'Type':'Image','Name':'Background','Width':800,'Height':480,'RelativeFilename':'bg_settings_800x480.png'},
 text('Title','TXT_SEQUENCER_TITLE',28,17,450,36),button('BackButton','TXT_SETTINGS_BACK',628,10),
 text('TableHeader','TXT_SEQ_HEADER',42,72,716,22,(147,163,184))]
for n,y in enumerate((96,154,212,270),1):s['Components']+=row(n,y)
s['Components'] += [
 text('SelectedLabel','TXT_SEQ_SELECTED',34,337,260,22,(42,199,217),'TXT_SEQ_SELECTED_INITIAL'),
 button('VoltageDownButton','TXT_V_MINUS',30,365,112),button('VoltageUpButton','TXT_V_PLUS',148,365,112),
 button('CurrentDownButton','TXT_I_MINUS',266,365,112),button('CurrentUpButton','TXT_I_PLUS',384,365,112),
 button('TimeButton','TXT_TIME_NEXT',502,365,112),button('SlewButton','TXT_SLEW_NEXT',620,365,150),
 button('EnableButton','TXT_STEP_TOGGLE_SHORT',25,418,140),button('AddStepButton','TXT_ADD_STEP',175,418,140),
 button('RemoveStepButton','TXT_REMOVE_STEP',325,418,140),button('LoopButton','TXT_LOOP_MODE',475,418,140),
 button('RunButton','TXT_RUN_SEQUENCE',625,418,150),
 text('RunStatus','TXT_SEQ_STATUS',300,337,465,22,(54,215,137),'TXT_SEQ_STATUS_READY')]
s['Interactions'] += [
 {'InteractionName':'addStepInteraction','Trigger':{'Type':'TriggerClicked','TriggerComponent':'AddStepButton'},'Action':{'Type':'ActionCustom','FunctionName':'addStep'}},
 {'InteractionName':'removeStepInteraction','Trigger':{'Type':'TriggerClicked','TriggerComponent':'RemoveStepButton'},'Action':{'Type':'ActionCustom','FunctionName':'removeStep'}},
 {'InteractionName':'loopModeInteraction','Trigger':{'Type':'TriggerClicked','TriggerComponent':'LoopButton'},'Action':{'Type':'ActionCustom','FunctionName':'nextLoopMode'}}]
pp.write_text(json.dumps(p,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')

tree=ET.parse(tp);g=tree.find('./Texts/TextGroup')
vals={'TXT_EMPTY':('Center','Small',' '),'TXT_SEQ_HEADER':('Left','Small','STEP       VOLTAGE       CURRENT       TIME       SLEW          STATE'),
'TXT_SEQ_ROW_VALUE':('Left','Button','<value>'),'TXT_SEQ_ROW1_INITIAL':('Left','Button','01       5.000 V       1.000 A       2.0 S      1.0 V/S       ENABLED'),
'TXT_SEQ_ROW2_INITIAL':('Left','Button','02      12.00 V        2.000 A       5.0 S      0.5 V/S       ENABLED'),
'TXT_SEQ_ROW3_INITIAL':('Left','Button','03      20.00 V        3.000 A       3.0 S      1.0 V/S       ENABLED'),
'TXT_SEQ_ROW4_INITIAL':('Left','Button','04       0.000 V       0.000 A       1.0 S      FAST          SKIPPED'),
'TXT_SEQ_SELECTED':('Left','Small','<value>'),'TXT_SEQ_SELECTED_INITIAL':('Left','Small','EDITING STEP 1'),
'TXT_V_MINUS':('Center','Button','V -'),'TXT_V_PLUS':('Center','Button','V +'),'TXT_I_MINUS':('Center','Button','I -'),'TXT_I_PLUS':('Center','Button','I +'),
'TXT_STEP_TOGGLE_SHORT':('Center','Button','ENABLE'),'TXT_ADD_STEP':('Center','Button','+ STEP'),
'TXT_REMOVE_STEP':('Center','Button','- STEP'),'TXT_LOOP_MODE':('Center','Button','LOOP')}
for tid,(a,t,v) in vals.items():
 n=next((x for x in g.findall('Text') if x.attrib['Id']==tid),None)
 if n is None:n=ET.SubElement(g,'Text',{'Id':tid,'Alignment':a,'TypographyId':t});tr=ET.SubElement(n,'Translation',{'Language':'GB'})
 else:tr=n.find('Translation')
 tr.text=v
ET.indent(tree,space='  ');tree.write(tp,encoding='utf-8',xml_declaration=True)
