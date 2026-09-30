"""Create visual/audio definitions for the ten additional gameplay entities.

Their ballistics are compiled C++; this file only supplies content resources.
"""
import argparse,json,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--stage',type=Path,required=True);p.add_argument('--unpacked',type=Path,required=True);a=p.parse_args()
rows=json.loads((a.stage/'arsenal_mapping.json').read_text())
for row in rows:
    if row['weapon_class']==row['script_source_class']:continue
    slug=Path(row['view_model']).stem.removeprefix('c_weapon_');source=a.unpacked/slug/'scripts'/(row['script_source_class']+'.txt')
    if not source.exists():source=a.unpacked/'csgo_mod_---_scripts_and_sounds/scripts'/(row['script_source_class']+'.txt')
    text=source.read_text(encoding='utf-8-sig')
    fields={'printname':row['name'],'viewmodel':row['view_model'],'playermodel':row['world_model'],'worldmodel':row['world_model'],'Team':'ANY'}
    for key,value in fields.items():
        pattern=r'("'+re.escape(key)+r'"\s+)"[^"]*"'
        if re.search(pattern,text,re.I):text=re.sub(pattern,lambda m:m[1]+'"'+value+'"',text,flags=re.I)
    (a.stage/row['script']).write_text(text,encoding='utf-8')
print('Additional gameplay definition files',sum(x['weapon_class']!=x['script_source_class'] for x in rows))
