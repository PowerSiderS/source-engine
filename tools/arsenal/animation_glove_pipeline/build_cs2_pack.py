import os
"""Compile supplied Source 1 ports without inventing or retiming motion."""
from pathlib import Path
import concurrent.futures,hashlib,json,re,shutil,subprocess,sys
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));root=work/'cs2-animation-import'
source=Path(r'C:\Users\SnyX\Downloads\Compressed\cs2 pack v1\cs2 test pack')
repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
game=Path(r'C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)')
stage=root/'assets/cstrike';stage.mkdir(parents=True,exist_ok=True)
(stage/'gameinfo.txt').write_text('"GameInfo" { game "CS2 pack authoring" FileSystem { SteamAppId 243750 SearchPaths { game+mod+mod_write+default_write_path |gameinfo_path|. game "C:/Program Files (x86)/Steam/steamapps/common/Source SDK Base 2013 Multiplayer/hl2" } } }')
sys.path.insert(0,str(repo/'tools/arsenal'))
from collect_materials import Resources
from collect_sounds import blocks as sounds
from port_catalog import blocks,rename_bones
resources=Resources([source,game/'cstrike',game/'hl2'])
vpk={'__file__':str(work/'extract_native_knives.py')}
exec((work/'extract_native_knives.py').read_text().split('selected=')[0],vpk)
copied={};missing=[];definitions={};sound_output={};sound_files=set();sound_fallbacks=[]
# Set SA_CS2_PACK_MODELS to a comma-separated list for an isolated compile.
# This lets a new viewmodel be inspected before the normal catalogue build.
selected_models={name.strip() for name in os.environ.get('SA_CS2_PACK_MODELS','').split(',') if name.strip()}
fields={'$basetexture','$bumpmap','$normalmap','$phongexponenttexture','$envmapmask','$detail','$lightwarptexture','$blendmodulatetexture','$mraotexture','$selfillummask'}
def collect(name):
 name=name.replace('\\','/').lower();target='materials/sourceadvanced/cs2_pack/'+name.removeprefix('materials/')
 if target in copied:return target.removeprefix('materials/').removesuffix('.vmt').removesuffix('.vtf')
 data=resources.get(name)
 if data is None:missing.append(name);return name.removeprefix('materials/').removesuffix('.vmt').removesuffix('.vtf')
 copied[target]=hashlib.sha256(data).hexdigest()
 if name.endswith('.vmt'):
  text=re.sub(r'//[^\r\n]*','',data.decode('utf-8-sig',errors='replace'))
  text=text.replace('$phongexponentfactor "','$phongexponentfactor" "')
  if '$no_draw' in text and 'pist_glock18/glock"' in text:
   text=text.replace('pist_glock18/glock"','pist_glock18/glock_color"')
  def replace(m):
   field=m[1];value=(m[2] or m[3]).replace('\\','/')
   if value.startswith(('_rt_','[')) or value.lower() in ('env_cubemap','env_cubemap_hdr'):return m[0]
   if field.lower() in fields:value=collect('materials/'+value.removesuffix('.vtf')+'.vtf')
   elif field.lower()=='include':value='materials/'+collect(('' if value.startswith('materials/') else 'materials/')+value.removesuffix('.vmt')+'.vmt')+'.vmt'
   else:return m[0]
   return '"'+field+'" "'+value+'"'
  data=re.sub(r'"?(\$\w+|include)"?\s+(?:"([^"\r\n]+)"|([^\s{}]+))',replace,text,flags=re.I).encode('utf8')
 dest=stage/target;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
 return target.removeprefix('materials/').removesuffix('.vmt').removesuffix('.vtf')
for p in sorted((source/'scripts').glob('game_sounds*.txt'),key=lambda p:('cs2' in p.name,p.name)):
 for key,body in sounds(p.read_text(encoding='utf-8-sig',errors='replace')):definitions.setdefault(key.lower(),[]).insert(0,(key,body,p.name))
csso=Path(r'C:\Users\SnyX\Downloads\Compressed\csso_release_1.1\csso')
for p in sorted((csso/'scripts').rglob('game_sounds*.txt')):
 for key,body in sounds(p.read_text(encoding='utf-8-sig',errors='replace')):definitions.setdefault(key.lower(),[]).append((key,body,'CSSO:'+p.name))
for name in vpk['entries']:
 if name.startswith('scripts/') and 'game_sounds' in name and name.endswith('.txt'):
  for key,body in sounds(vpk['payload'](name).decode('utf-8-sig',errors='replace')):definitions.setdefault(key.lower(),[]).append((key,body,'CSSO:'+name))
def audio_exists(value):
 clean=value.replace('\\','/').lstrip('*!#><^@)~?}')
 return any((base/'sound'/clean).is_file() for base in (source,game/'cstrike',csso)) or 'sound/'+clean in vpk['entries']
def sound_event(name):
 alias='SACS2.'+name
 if alias in sound_output:return alias
 choices=definitions.get(name.lower(),[])
 entry=next((e for e in choices if all(audio_exists(v) for v in re.findall(r'"?wave"?\s+"([^"\r\n]+)"',e[1],re.I))),None)
 if entry is None:missing.append('sound-event:'+name);return name
 if choices and entry!=choices[0]:sound_fallbacks.append(dict(event=name,source=entry[2]))
 body=entry[1]
 def wave(m):
  value=m[1].replace('\\','/');clean=value.lstrip('*!#><^@)~?}')
  prefix=''.join(c for c in value[:len(value)-len(clean)] if c in '*!#><^@)?}')
  file=source/'sound'/clean
  if not file.is_file():
   file=game/'cstrike/sound'/clean
  if not file.is_file():file=csso/'sound'/clean
  if file.is_file():audio=file.read_bytes()
  elif 'sound/'+clean in vpk['entries']:audio=vpk['payload']('sound/'+clean)
  else:missing.append('sound-file:'+clean);return m[0]
  new='sourceadvanced/cs2_pack/'+clean;dest=stage/'sound'/new;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(audio);sound_files.add(new)
  return '"wave" "'+prefix+new+'"'
 body=re.sub(r'"?wave"?\s+"([^"\r\n]+)"',wave,body,flags=re.I)
 # Source 2 sound operator stacks are metadata, not implemented by Source 1.
 # Only copy supported Source 1 sound properties and the actual wave choices.
 props=[]
 for field in ('channel','volume','pitch','soundlevel'):
  match=re.search(r'"'+field+r'"\s+"([^"\r\n]+)"',body,re.I)
  if match:props.append('"'+field+'" "'+match[1]+'"')
 waves=re.findall(r'"wave"\s+"([^"\r\n]+)"',body,re.I)
 if len(waves)==1:props.append('"wave" "'+waves[0]+'"')
 elif waves:props.append('"rndwave" { '+ ' '.join('"wave" "'+v+'"' for v in waves)+' }')
 sound_output[alias]='"'+alias+'"\n{\n'+'\n'.join(props)+'\n}\n';return alias
audit=[];qcs=[]
for original in sorted((root/'decompiled').glob('*/')):
 # The supplied Source 1 port includes firearms and the CS2 Butterfly. Do not
 # use the Butterfly clips on another knife: blade pivots and both arm chains
 # are part of this rig.
 if not original.name.startswith(('v_pist_','v_rif_','v_snip_','v_shot_','v_smg_','v_mach_','v_knife_t')):continue
 if selected_models and original.name not in selected_models:continue
 dst=root/'authored'/original.name;shutil.copytree(original,dst,dirs_exist_ok=True)
 qc=next(dst.glob('*.qc'));before=qc.read_text(encoding='utf-8-sig');text=rename_bones(before)
 text=re.sub(r'\$modelname\s+"[^"]+"',lambda m:'$modelname "sourceadvanced/cs2/'+original.name+'.mdl"',text,count=1)
 dirs=re.findall(r'\$cdmaterials\s+"([^"]+)"',text)
 meshes=re.findall(r'studio\s+"([^"\r\n]+\.smd)"',text)
 for mesh in dst.rglob('*.smd'):
  meshtext=mesh.read_text(encoding='utf-8-sig');mesh.write_text(rename_bones(meshtext),encoding='utf8')
  if 'triangles\n' in meshtext:
   for material in set(meshtext.split('triangles\n')[1].splitlines()[:-1:4]):
    candidate=next(('materials/'+d.replace('\\','/').strip('/')+'/'+material+'.vmt' for d in dirs if resources.get('materials/'+d.replace('\\','/').strip('/')+'/'+material+'.vmt') is not None),None)
    if candidate:collect(candidate)
    else:missing.append(original.name+':material:'+material)
 for directory in dirs:
  text=text.replace('$cdmaterials "'+directory+'"','$cdmaterials "sourceadvanced/cs2_pack/'+directory.replace('\\','/').strip('/')+'/"')
 # Give embedded arms explicit, hideable groups for the optional glove system.
 arms=[]
 for index,(name,block) in enumerate(list(blocks(text,'bodygroup'))):
  mesh=re.search(r'studio\s+"([^"]+)"',block)
  if mesh and (any(word in (name+' '+mesh[1]).lower() for word in ('glove','arm','hands')) or Path(mesh[1]).stem.lower()=='sport'):
   group='sa_embedded_arms_'+str(len(arms));arms.append(group)
   replacement=block.replace('"'+name+'"','"'+group+'"',1)
   if 'blank' not in replacement:replacement=replacement.rsplit('}',1)[0]+'\n blank\n}'
   text=text.replace(block,replacement,1)
 # These numeric options are muzzle attachment indices in the supplied
 # Five-Seven/Elite shoot variants, not names of sound entries.
 text=re.sub(r'(event\s+)5004(\s+\d+\s+"[12]")',r'\g<1>5001\g<2>',text)
 text=re.sub(r'(event\s+5004\s+\d+\s+)"([^"\n]+)"',lambda m:m[1]+'"'+sound_event(m[2])+'"',text)
 # Translate known animation event names, without changing frames or motion.
 text=text.replace('AE_WPN_INSPECTION_LOOP','AE_BEGIN_TAUNT_LOOP')
 text=re.sub(r'(event\s+)(AE_CLIENT_EJECT_BRASS|AE_WPN_EJECT_BRASS)(\s+\d+\s+)"[^"]*"',r'\g<1>6001\g<3>"0"',text)
 stripped=[]
 supported=(repo/'game/shared/eventlist.h').read_text()
 def unsupported(m):
  if m[1] in supported:return m[0]
  stripped.append(m[1]);return ''
 text=re.sub(r'\{\s*event\s+(AE_\w+)\s+[^{}]+\}',unsupported,text)
 qc.write_text(text,encoding='utf8');qcs.append(qc)
 animations=[p for p in dst.rglob('*.smd') if 'triangles\n' not in p.read_text(encoding='utf8')]
 # Renaming bone identities is allowed; every numeric animation frame must match.
 for anim in animations:
  orig=original/anim.relative_to(dst)
  if rename_bones(orig.read_text(encoding='utf-8-sig'))!=anim.read_text(encoding='utf8'):raise RuntimeError('Motion modified: '+str(anim))
 audit.append(dict(model=original.name,view_model='models/sourceadvanced/cs2/'+original.name+'.mdl',embedded_arms=arms,source_sequences=[n for n,_ in blocks(before,'sequence')],animation_files=len(animations),numeric_motion_preserved=True,unsupported_metadata_events=sorted(set(stripped))))
scripts=stage/'scripts';scripts.mkdir(exist_ok=True)
(scripts/'game_sounds_sourceadvanced_cs2.txt').write_text('\n'.join(sound_output.values()),encoding='utf8')
(root/'asset-audit.json').write_text(json.dumps(dict(models=audit,materials=len(copied),sound_events=len(sound_output),sound_files=len(sound_files),sound_fallbacks=sound_fallbacks,missing=sorted(set(missing))),indent=2),encoding='utf8')
if missing:print('Missing dependencies:',sorted(set(missing)),flush=True);raise SystemExit(1)
exe=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\bin\studiomdl.exe')
def compile(qc):
 result=subprocess.run([str(exe),'-game',str(stage),'-nop4',str(qc)],capture_output=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW)
 (qc.parent/'compile.log').write_bytes(result.stdout+result.stderr)
 ok=not result.returncode and (stage/'models/sourceadvanced/cs2'/(qc.parent.name+'.mdl')).is_file()
 print(qc.parent.name,'PASS' if ok else 'FAIL',flush=True);return ok
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:results=list(pool.map(compile,qcs))
if not all(results):raise SystemExit(1)
print('Compiled',len(qcs),'supplied CS2 viewmodels with original motion.',flush=True)
