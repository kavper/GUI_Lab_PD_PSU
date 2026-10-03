import json
from pathlib import Path
import xml.etree.ElementTree as ET
r=Path(__file__).resolve().parents[1];p=r/'Appli/TouchGFX/GUI_Lab_PD_PSU.touchgfx';t=r/'Appli/TouchGFX/assets/texts/texts.xml'
j=json.loads(p.read_text(encoding='utf-8-sig'));s=next(x for x in j['Application']['Screens'] if x['Name']=='ScreenSystem')
s['Components']=[x for x in s['Components'] if x.get('Name') not in ('NetworkLabel','NetworkValue')]
s['Components'] += [
 {'Type':'TextArea','Name':'NetworkLabel','X':42,'Y':365,'Width':210,'Height':24,'TextId':'TXT_NETWORK_LABEL','TextRotation':'0','Color':{'Red':147,'Green':163,'Blue':184}},
 {'Type':'TextArea','Name':'NetworkValue','X':250,'Y':360,'Width':500,'Height':32,'TextId':'TXT_NETWORK_VALUE','TextRotation':'0','Color':{'Red':42,'Green':199,'Blue':217},'Wildcard1':{'TextId':'TXT_NETWORK_INITIAL','UseBuffer':True,'BufferSize':32}}]
p.write_text(json.dumps(j,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
tree=ET.parse(t);g=tree.find('./Texts/TextGroup');vals={'TXT_NETWORK_LABEL':('Left','Button','ETHERNET IP'),'TXT_NETWORK_VALUE':('Left','Button','<value>'),'TXT_NETWORK_INITIAL':('Left','Button','STARTING NETWORK...')}
for tid,(a,typ,v) in vals.items():
 n=next((x for x in g.findall('Text') if x.attrib['Id']==tid),None)
 if n is None:n=ET.SubElement(g,'Text',{'Id':tid,'Alignment':a,'TypographyId':typ});tr=ET.SubElement(n,'Translation',{'Language':'GB'})
 else:tr=n.find('Translation')
 tr.text=v
ET.indent(tree,space='  ');tree.write(t,encoding='utf-8',xml_declaration=True)
