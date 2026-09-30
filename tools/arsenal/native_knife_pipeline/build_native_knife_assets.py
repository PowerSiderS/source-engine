import os
"""Compile existing knife clips. No motion or numeric SMD frames are authored."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import contextlib,io,json,re,shutil,subprocess,sys,hashlib
work=Path(os.environ.get('SA_KNIFE_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'))
root=work/'native-knife-engine-20260930'
source=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
game=Path(r'C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)')
stage=root/'assets/cstrike';stage.mkdir(parents=True,exist_ok=True)
(stage/'gameinfo.txt').write_text('"GameInfo" { game "Native knife authoring" FileSystem { SteamAppId 243750 SearchPaths { game+mod+mod_write+default_write_path |gameinfo_path|. game "C:/Program Files (x86)/Steam/steamapps/common/Source SDK Base 2013 Multiplayer/hl2" } } }')
sys.path.insert(0,str(source/'tools/arsenal'))
from collect_materials import Resources
from port_catalog import rename_bones,blocks
from collect_sounds import blocks as sound_blocks
resources=Resources([Path(r'C:\Users\SnyX\Downloads\Compressed\csso_release_1.1\csso')])
with contextlib.redirect_stdout(io.StringIO()):
 import extract_native_knives as vpk
mapping={
'bayonet':'bayonet','bowie_knife':'survival_bowie','butterfly_knife':'butterfly','classic_knife':'css',
'ct_knife':'ct','falchion_knife':'falchion_advanced','flip_knife':'flip','gut_knife':'gut','huntsman_knife':'tactical',
'karambit':'karam','m9_bayonet':'m9_bay','navaja_knife':'gypsy_jackknife','nomad_knife':'outdoor',
'paracord_knife':'cord','shadow_daggers':'push','skeleton_knife':'skeleton','stiletto_knife':'stiletto',
'survival_knife':'canis','t_knife':'t','talon_knife':'widowmaker','ursus_knife':'ursus'}
material_files={};missing=[]
texture_fields={'$basetexture','$bumpmap','$normalmap','$phongexponenttexture','$envmapmask','$detail','$lightwarptexture','$blendmodulatetexture'}
def collect(name):
 name=name.replace('\\','/').lower()
 target='materials/sourceadvanced/native_knives/'+name.removeprefix('materials/')
 if target in material_files:return target.removeprefix('materials/').removesuffix('.vmt').removesuffix('.vtf')
 data=resources.get(name)
 if data is None:raise RuntimeError('Missing native material dependency: '+name)
 material_files[target]=hashlib.sha256(data).hexdigest()
 if name.endswith('.vmt'):
  text=re.sub(r'//[^\r\n]*','',data.decode('utf-8-sig',errors='replace'))
  def replace(match):
   field,value=match.group(1),match.group(2)
   if field.lower() in texture_fields:
    new=collect('materials/'+value.replace('\\','/').removesuffix('.vtf')+'.vtf')
   elif field.lower()=='include':
    key=value.replace('\\','/');key=key if key.startswith('materials/') else 'materials/'+key
    new=collect(key.removesuffix('.vmt')+'.vmt')+'.vmt'
   elif field.lower()=='$envmap' and value.lower() not in ('env_cubemap','env_cubemap_hdr',''):
    new=collect('materials/'+value.replace('\\','/').removesuffix('.vtf')+'.vtf')
   else:return match.group(0)
   return '"'+field+'" "'+new+'"'
  text=re.sub(r'"?(\$\w+|include)"?\s+"([^"\r\n]+)"',replace,text,flags=re.I)
  data=text.encode('utf-8')
 dest=stage/target;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
 return target.removeprefix('materials/').removesuffix('.vmt').removesuffix('.vtf')
definitions={}
csso=Path(r'C:\Users\SnyX\Downloads\Compressed\csso_release_1.1\csso')
for file in sorted((csso/'scripts').rglob('game_sounds*.txt')):
 for key,body in sound_blocks(file.read_text(encoding='utf-8-sig',errors='replace')):definitions[key.lower()]=(key,body)
for name in sorted(vpk.entries):
 if name.startswith('scripts/') and 'game_sounds' in name and name.endswith('.txt'):
  for key,body in sound_blocks(vpk.payload(name).decode('utf-8-sig',errors='replace')):definitions[key.lower()]=(key,body)
sound_output={};sound_files=set()
def sound_event(name):
 name={'KnifeWidow.LookAt2Start':'Knife.Widow.LookAt2.Start','KnifeWidow.LookAt2End':'Knife.Widow.LookAt2.End'}.get(name,name)
 key=name.lower()
 alias='SANative.'+name
 if alias in sound_output:return alias
 if key not in definitions:
  missing.append('sound-event:'+name);return name
 _,body=definitions[key]
 def wave(match):
  value=match.group(1).replace('\\','/');clean=value.lstrip('*!#><^@)~?}');prefix=value[:len(value)-len(clean)]
  # This Source branch understands spatial stereo ')', but not the CS:GO
  # '~' metadata flag. Preserve supported routing without changing audio.
  prefix=''.join(c for c in prefix if c in '*!#><^@)?}')
  src='sound/'+clean
  loose=csso/src
  if src not in vpk.entries and not loose.is_file():missing.append('sound-file:'+src);return match.group(0)
  new='sourceadvanced/native_knives/'+clean
  dest=stage/'sound'/new;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(loose.read_bytes() if loose.is_file() else vpk.payload(src));sound_files.add(new)
  return '"wave" "'+prefix+new+'"'
 body=re.sub(r'"?wave"?\s+"([^"\r\n]+)"',wave,body,flags=re.I)
 sound_output[alias]='"'+alias+'"\n{'+body+'}\n'
 return alias
qcs=[];audit=[]
for slug,native in mapping.items():
 src=root/'decompiled'/('v_knife_'+native)
 dst=root/'authored'/('knife_'+slug)
 shutil.copytree(src,dst,dirs_exist_ok=True)
 qc=next(dst.glob('*.qc'));text=qc.read_text(encoding='utf-8-sig')
 text=re.sub(r'\$modelname\s+"[^"]+"',lambda m:'$modelname "sourceadvanced/c_weapon_knife_'+slug+'.mdl"',text,count=1)
 directories=re.findall(r'\$cdmaterials\s+"([^"]+)"',text)
 for mesh in dst.glob('*.smd'):
  value=mesh.read_text(encoding='utf-8-sig')
  if 'triangles\n' in value:
   mats=set(value.split('triangles\n')[1].splitlines()[:-1:4])
   for material in mats:
    candidate=next(('materials/'+directory.replace('\\','/').strip('/')+'/'+material+'.vmt' for directory in directories
                   if resources.get('materials/'+directory.replace('\\','/').strip('/')+'/'+material+'.vmt') is not None),None)
    if not candidate:raise RuntimeError('No material: '+slug+':'+material)
    collect(candidate)
 for directory in directories:
  new='sourceadvanced/native_knives/'+directory.replace('\\','/').strip('/')+'/'
  text=text.replace('$cdmaterials "'+directory+'"','$cdmaterials "'+new+'"')
 text=re.sub(r'(event\s+5004\s+\d+\s+)"([^"\n]+)"',lambda m:m.group(1)+'"'+sound_event(m.group(2))+'"',text)
 # Only node names change. Numeric transforms, frame counts, timing and events
 # come from the existing native clips.
 clips=[]
 for smd in dst.rglob('*.smd'):
  original=smd.read_text(encoding='utf-8-sig');ported=rename_bones(original)
  original_frames=original.split('skeleton\n',1)[1].split('\nend',1)[0]
  assert original_frames==ported.split('skeleton\n',1)[1].split('\nend',1)[0]
  smd.write_text(ported,encoding='utf-8')
  if '_anims' in smd.parent.name:clips.append(dict(file=smd.name,frames_unchanged=True,sha256=hashlib.sha256(original_frames.encode()).hexdigest()))
 text=rename_bones(text)
 # The native models intentionally leave several finger bones unused by the
 # blade mesh. Keep every arm bone for the bonemerged glove mesh.
 bone_names=re.findall(r'\$definebone\s+"([^"]*Bip01[^"]*)"',text)
 text+='\n'+''.join('$bonemerge "'+name+'"\n' for name in bone_names)
 qc.write_text(text,encoding='utf-8');qcs.append(qc)
 sequences=[name for name,_ in blocks(text,'sequence')]
 assert 'lookat01' in sequences and 'light_miss1' in sequences and 'heavy_miss1' in sequences
 audit.append(dict(catalog='knife_'+slug,native_model='v_knife_'+native,clips=clips,sequences=sequences))
arms=root/'authored/arms'
subprocess.run([sys.executable,str(source/'tools/arsenal/import_embedded_arms.py'),
 '--mesh',str(work/'arms-from-glock-20260930/decompiled/v_gloves.smd'),'--output',str(arms),
 '--keep-twists','--model-name','c_arms_native'],check=True,stdout=(root/'arms-import.log').open('w'))
qcs.append(arms/'c_arms_default.qc')
compiler=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\bin\studiomdl.exe')
def compile(qc):
 r=subprocess.run([str(compiler),'-game',str(stage),'-nop4',str(qc)],cwd=compiler.parent,
                  stdout=subprocess.PIPE,stderr=subprocess.STDOUT,creationflags=0x08000000)
 qc.with_suffix('.compile.log').write_bytes(r.stdout)
 if r.returncode or b'ERROR:' in r.stdout:raise RuntimeError('Compile failed: '+str(qc))
 return qc.parent.name
with ThreadPoolExecutor(max_workers=2) as pool:
 for compiled in pool.map(compile,qcs):print('Compiled native',compiled,flush=True)
(stage/'scripts').mkdir(exist_ok=True)
(stage/'scripts/game_sounds_sourceadvanced_native_knives.txt').write_text(''.join(sound_output.values()),encoding='utf-8')
manifest=(game/'cstrike/scripts/skins_manifest.txt').read_text()
def patch_item(match):
 body=match.group(2)
 if '"weapon_knife"' not in body:return match.group(0)
 body=re.sub(r'("sound_script"\s+)"[^"]*"',r'\1"scripts/game_sounds_sourceadvanced_native_knives.txt"',body)
 return match.group(1)+body+'}'
manifest=re.sub(r'("\d+"\s*\{)([^}]+)\}',patch_item,manifest)
(stage/'scripts/skins_manifest.txt').write_text(manifest,encoding='utf-8')
report=dict(models=audit,materials=len(material_files),sounds=len(sound_output),audio_files=len(sound_files),missing=missing,
            animation_source='Existing CSSO native CSGO knife clips; numeric frames unchanged; no authored motion')
(root/'asset-validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('Native clips compiled:',len(audit),'materials:',len(material_files),'missing sounds:',missing,flush=True)
