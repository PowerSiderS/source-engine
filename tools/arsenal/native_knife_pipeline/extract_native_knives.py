import os
from pathlib import Path
import json, struct, zlib, sys
root=Path(os.environ.get('SA_KNIFE_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'))/'native-knife-engine-20260930'
root.mkdir(exist_ok=True)
archive=Path(r'C:\Users\SnyX\Downloads\Compressed\csso_release_1.1\csso\csso_pak_dir.vpk')
data=archive.read_bytes(); _,version,tree=struct.unpack_from('<III',data)
pos=12 if version==1 else 28; base=pos+tree; entries={}
def string():
 global pos
 end=data.index(0,pos); value=data[pos:end].decode(); pos=end+1; return value
while ext:=string():
 while folder:=string():
  while stem:=string():
   crc,pre,part,offset,size,terminator=struct.unpack_from('<IHHIIH',data,pos); pos+=18
   preload=data[pos:pos+pre]; pos+=pre
   name=('' if folder==' ' else folder+'/')+stem+'.'+ext
   entries[name]=(crc,preload,part,offset,size)
def payload(name):
 crc,pre,part,offset,size=entries[name]
 if part==32767: result=pre+data[base+offset:base+offset+size]
 else:
  with archive.with_name(archive.name.replace('_dir.vpk',f'_{part:03d}.vpk')).open('rb') as f:
   f.seek(offset); result=pre+f.read(size)
 if zlib.crc32(result)!=crc: raise ValueError('CRC: '+name)
 return result
selected=[name for name in entries if name.startswith('models/weapons/v_knife_') and name.endswith('.mdl')]
# Decompilation needs included animation libraries as well as mesh companions.
selected+= [name for name in entries if name.startswith('models/weapons/') and 'anim' in name and name.endswith('.mdl')]
copied=[]
for name in selected:
 stem=name[:-4]
 for sibling in [n for n in entries if n==name or n.startswith(stem+'.') or n==stem+'.ani']:
  if not sibling.endswith(('.mdl','.vvd','.vtx','.ani','.phy')):continue
  dest=root/'csso-native'/sibling; dest.parent.mkdir(parents=True,exist_ok=True)
  dest.write_bytes(payload(sibling)); copied.append(sibling)
sys.path.insert(0,str(Path(os.environ.get('SA_KNIFE_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'))))
from inspect_source_mdl import inspect
audit=[inspect(root/'csso-native'/name) for name in selected if 'v_knife_' in name]
(root/'native-model-audit.json').write_text(json.dumps(audit,indent=2),encoding='utf-8')
for item in audit:print(Path(item['file']).name,item['version'],item['numbones'],[s.get('name',s.get('label')) for s in item['sequences']])
print('Extracted',len(set(copied)),'CRC-verified files')
