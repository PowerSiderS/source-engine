import os
"""Use CSSO glove geometry/UVs; rebind by anatomy to three viewmodel rigs."""
from pathlib import Path
import concurrent.futures,hashlib,json,re,subprocess,sys
import numpy as np
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));root=work/'csso-glove-import';repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
stage=root/'assets/cstrike';stage.mkdir(parents=True,exist_ok=True)
(stage/'gameinfo.txt').write_text('"GameInfo" { game "CSSO glove authoring" FileSystem { SteamAppId 243750 SearchPaths { game+mod+mod_write+default_write_path |gameinfo_path|. game "C:/Program Files (x86)/Steam/steamapps/common/Source SDK Base 2013 Multiplayer/hl2" } } }')
sys.path.insert(0,str(repo/'tools/arsenal'))
from port_catalog import nodes,first_pose,global_pose,rename_bones
from collect_materials import Resources
resources=Resources([Path(r'C:\Users\SnyX\Downloads\Compressed\csso_release_1.1\csso')])
copied={};missing=[]
fields={'$basetexture','$bumpmap','$normalmap','$phongexponenttexture','$envmapmask','$detail','$lightwarptexture','$blendmodulatetexture','$selfillummask','$phongwarptexture','$masks1','$fresnelrangestexture'}
def collect(name):
 name=name.replace('\\','/').lower();target='materials/sourceadvanced/csso_gloves/'+name.removeprefix('materials/')
 # ASCII namespace, independent of original game materials.
 if target in copied:return target.removeprefix('materials/').removesuffix('.vmt').removesuffix('.vtf')
 data=resources.get(name)
 if data is None:missing.append(name);return name.removeprefix('materials/').removesuffix('.vmt').removesuffix('.vtf')
 copied[target]=hashlib.sha256(data).hexdigest()
 if name.endswith('.vmt'):
  text=re.sub(r'//[^\r\n]*','',data.decode('utf-8-sig',errors='replace'))
  # Character is a CSSO shader absent in this Source build. Retain the
  # original color/normal textures and use its supported model shader.
  text=re.sub(r'^\s*"?character"?(?=\s*\{)','"VertexLitGeneric"',text,flags=re.I)
  def patch(m):
   field=m[1];value=(m[2] or m[3]).replace('\\','/')
   if value.startswith(('_rt_','[')) or value.lower() in ('env_cubemap','env_cubemap_hdr'):return m[0]
   if field.lower() in fields:value=collect('materials/'+value.removesuffix('.vtf')+'.vtf')
   elif field.lower()=='include':value='materials/'+collect(('' if value.startswith('materials/') else 'materials/')+value.removesuffix('.vmt')+'.vmt')+'.vmt'
   else:return m[0]
   return '"'+field+'" "'+value+'"'
  data=re.sub(r'"?(\$\w+|include)"?\s+(?:"([^"\r\n]+)"|([^\s{}]+))',patch,text,flags=re.I).encode('utf8')
 dest=stage/target;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
 return target.removeprefix('materials/').removesuffix('.vmt').removesuffix('.vtf')
refs={
 'native':work/'native-knife-engine-20260930/authored/arms/c_arms_default.smd',
 'legacy':work/'arsenal-ported/arms/c_arms_default.smd',
 'cs2':work/'cs2-animation-import/decompiled/v_rif_ak47/cs2_sporty_gloves.smd'}

def anatomical_frame(world,side,cs2,forearm=False):
 def joint(part):
  name=part+'_'+side if cs2 else 'ValveBiped.Bip01_'+side+'_'+part
  return world[name][:3,3]
 hand=joint('hand' if cs2 else 'Hand')
 index=joint('finger_index_0' if cs2 else 'Finger1')
 pinky=joint('finger_pinky_0' if cs2 else 'Finger4')
 middle=joint('finger_middle_0' if cs2 else 'Finger2')
 shaft=hand-joint('arm_lower' if cs2 else 'Forearm') if forearm else middle-hand
 shaft=shaft/np.linalg.norm(shaft)
 radial=index-pinky;radial-=shaft*np.dot(shaft,radial);radial/=np.linalg.norm(radial)
 return np.column_stack((shaft,radial,np.cross(shaft,radial)))
def target_name(name,rig):
 name=rename_bones(name)
 if rig=='legacy':return name.replace('_ForeTwist','_Forearm')
 if rig=='native':return name
 match=re.fullmatch(r'ValveBiped\.Bip01_([LR])_(Hand|Forearm|ForeTwist|Finger([0-4])([12]?))',name)
 if not match:raise ValueError('Unsupported weighted CS2 mapping: '+name)
 side,part,digit,segment=match.groups()
 if part=='Hand':return 'hand_'+side
 if part in ('Forearm','ForeTwist'):return 'arm_lower_'+side+'_TWIST1'
 finger=('thumb','index','middle','ring','pinky')[int(digit)]
 return 'finger_'+finger+'_'+str(int(segment or 0))+'_'+side
materials_only='--materials-only' in sys.argv
only_rig=next((a.split('=',1)[1] for a in sys.argv if a.startswith('--rig=')),None)
only_style=next((a.split('=',1)[1] for a in sys.argv if a.startswith('--style=')),None)
qcs=[];catalog=[];bind_audit=json.loads((root/'asset-audit.json').read_text())['bind_audit'] if (materials_only or only_rig or only_style) and (root/'asset-audit.json').exists() else []
directories=[p for p in sorted((root/'decompiled').iterdir()) if p.name.startswith('v_glove_') or p.name=='v_bare_hands']
for item_id,directory in enumerate(directories,4000):
 qc=next(directory.glob('*.qc')).read_text(encoding='utf-8-sig')
 meshes=[directory/n for n in re.findall(r'studio\s+"([^"\r\n]+\.smd)"',qc)]
 dirs=re.findall(r'\$cdmaterials\s+"([^"\r\n]+)"',qc)
 for mesh in meshes:
  text=mesh.read_text(encoding='utf-8-sig')
  for material in set(text.split('triangles\n')[1].splitlines()[:-1:4]):
   path=next(('materials/'+d.replace('\\','/').strip('/')+'/'+material+'.vmt' for d in dirs if resources.get('materials/'+d.replace('\\','/').strip('/')+'/'+material+'.vmt') is not None),None)
   if path:collect(path)
   else:missing.append(directory.name+':'+material)
 # Skin families select the CSSO bare-arm tones; collect their extra textures.
 skinblock=re.search(r'\$texturegroup\s+"[^"\n]+"\s*\{(.*?)\n\}',qc,re.S)
 if skinblock:
  for material in re.findall(r'"([^"\n]+)"',skinblock[1]):
   path=next(('materials/'+d.replace('\\','/').strip('/')+'/'+material+'.vmt' for d in dirs if resources.get('materials/'+d.replace('\\','/').strip('/')+'/'+material+'.vmt') is not None),None)
   if path:collect(path)
 paths={}
 for rig,reference in refs.items():
  if materials_only or only_rig and only_rig!=rig or only_style and only_style!=directory.name:
   paths[rig]='models/sourceadvanced/gloves/'+directory.name+'_'+rig+'.mdl'
   continue
  ref=rename_bones(reference.read_text(encoding='utf-8-sig'));ref_nodes=nodes(ref);ref_pose=first_pose(ref);ref_world=global_pose(ref)
  names={n:i for i,n,p in ref_nodes};parents={i:p for i,n,p in ref_nodes}
  triangles=[];used=set();sources=[]
  for mesh in meshes:
   text=rename_bones(mesh.read_text(encoding='utf-8-sig'));source_nodes=nodes(text);source_names={i:n for i,n,p in source_nodes};source_world=global_pose(text)
   lines=text.split('triangles\n')[1].splitlines();transforms={};mapping={}
   for offset in range(0,len(lines)-1,4):
    if lines[offset]=='end':break
    vertices=[]
    for line in lines[offset+1:offset+4]:
     parts=line.split();links=int(parts[9]) if len(parts)>9 else 0
     weights=[(int(parts[10+i*2]),float(parts[11+i*2])) for i in range(links)] or [(int(parts[0]),1.)]
     weights=[(i,w) for i,w in weights if w>0];total=sum(w for i,w in weights)
     if abs(total-1)>1e-4:raise ValueError('Invalid source weights')
     point=np.array([*map(float,parts[1:4]),1.]);normal=np.array(list(map(float,parts[4:7])))
     positioned=np.zeros(4);oriented=np.zeros(3);new_weights={}
     for bone,weight in weights:
      if bone not in mapping:
       source_name=source_names[bone];target=target_name(source_name,rig)
       if target not in names:raise ValueError('Missing reference bone '+target)
       mapping[bone]=names[target]
       target_bind=ref_world[target].copy()
       if rig=='cs2':
        side=re.search(r'Bip01_([LR])_',source_name)[1]
        # The right CS2 chain uses proximal X and opposite Y. CSSO uses
        # distal X on both sides. Preserve anatomical handedness, not the
        # coincident bone label. The same correction applies to fingers.
        correction=np.diag([-1.,-1.,1.]) if side=='R' else np.eye(3)
        target_bind[:3,:3]=target_bind[:3,:3]@correction
        if source_name.endswith(('_Forearm','_ForeTwist')):
         # CSSO's twist pivot is at the elbow; CS2's TWIST1 pivot is 2/3
         # down the arm. Anchor the geometry at the wrist instead of
         # translating the CSSO elbow onto the different twist pivot.
         src_hand=source_world['ValveBiped.Bip01_'+side+'_Hand'][:3,3]
         dst_hand=ref_world['hand_'+side][:3,3]
         # Match the wrist's anatomical cross-section too. A raw bind
         # rotation has a different palm roll on the two rigs; blending
         # that with hand weights bakes a collapsed cuff into the mesh.
         rot=anatomical_frame(ref_world,side,True,True)@anatomical_frame(source_world,side,False,True).T
         matrix=np.eye(4);matrix[:3,:3]=rot
         matrix[:3,3]=dst_hand-rot@src_hand
        elif source_name.endswith('_Hand'):
         rot=anatomical_frame(ref_world,side,True)@anatomical_frame(source_world,side,False).T
         matrix=np.eye(4);matrix[:3,:3]=rot
         matrix[:3,3]=ref_world[target][:3,3]-rot@source_world[source_name][:3,3]
        else:matrix=target_bind@np.linalg.inv(source_world[source_name])
       else:matrix=target_bind@np.linalg.inv(source_world[source_name])
       transforms[bone]=matrix
      matrix=transforms[bone];target=mapping[bone];used.add(target)
      positioned+=weight*(matrix@point);oriented+=weight*(matrix[:3,:3]@normal)
      new_weights[target]=new_weights.get(target,0)+weight
     length=np.linalg.norm(oriented)
     if not np.isfinite(positioned).all() or length<1e-9:raise ValueError('Invalid bind-space geometry')
     vertices.append((positioned[:3],oriented/length,parts[7:9],new_weights))
    triangles.append((lines[offset],vertices))
   sources.append(dict(file=str(mesh),sha256=hashlib.sha256(mesh.read_bytes()).hexdigest()))
  for bone in list(used):
   parent=parents[bone]
   while parent>=0:used.add(parent);parent=parents[parent]
  selected=[(i,n,p) for i,n,p in ref_nodes if i in used];indices={i:new for new,(i,n,p) in enumerate(selected)}
  output=['version 1','nodes']+[f'{indices[i]} "{n}" {indices.get(p,-1)}' for i,n,p in selected]
  output+=['end','skeleton','time 0']+[str(indices[i])+' '+' '.join(f'{v:.9f}' for v in ref_pose[i]) for i,n,p in selected]+['end','triangles']
  for material,vertices in triangles:
   output.append(material)
   for position,normal,uv,weights in vertices:
    ordered=sorted(((indices[i],w) for i,w in weights.items()),key=lambda p:-p[1])
    output.append(str(ordered[0][0])+' '+' '.join(f'{v:.9f}' for v in [*position,*normal])+' '+' '.join(uv)+' '+str(len(ordered))+' '+' '.join(f'{i} {w:.9f}' for i,w in ordered))
  output.append('end');dst=root/'authored'/directory.name/rig;dst.mkdir(parents=True,exist_ok=True)
  (dst/'arms.smd').write_text('\n'.join(output)+'\n',encoding='utf8')
  path='sourceadvanced/gloves/'+directory.name+'_'+rig+'.mdl';paths[rig]='models/'+path
  qcout=[f'$modelname "{path}"','$body "arms" "arms.smd"','$surfaceprop "flesh"','$sequence "idle" "arms.smd" fps 1 loop']
  qcout+=['$cdmaterials "sourceadvanced/csso_gloves/'+d.replace('\\','/').strip('/')+'/"' for d in dirs]
  qcout+=['$bonemerge "'+n+'"' for i,n,p in selected]
  if skinblock:qcout.append(skinblock[0])
  qcp=dst/'gloves.qc';qcp.write_text('\n'.join(qcout)+'\n',encoding='utf8');qcs.append(qcp)
  bind_audit=[a for a in bind_audit if (a['style'],a['rig'])!=(directory.name,rig)]
  bind_audit.append(dict(style=directory.name,rig=rig,triangles=len(triangles),bones=[n for i,n,p in selected],source_meshes=sources,uvs_preserved=True,policy='CSSO anatomical wrist/palm frames and wrist-anchored TWIST1 for CS2; semantic joints for original rigs; weapon animation motion unchanged',binding_version=2))
 icon={'v_bare_hands':'glove_bare','v_glove_anarchist':'t_none'}.get(directory.name,directory.name.removeprefix('v_'))
 iconpath='materials/vgui/gloves/'+icon+'.vmt'
 if resources.get(iconpath) is not None:
  collected=collect(iconpath);payload=(stage/'materials'/(collected+'.vmt')).read_bytes()
  dest=stage/'materials/vgui/sourceadvanced/gloves'/(directory.name+'.vmt');dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(payload)
 else:missing.append('icon:'+directory.name)
 catalog.append(dict(item_id=item_id,name=directory.name.removeprefix('v_').replace('_',' ').title(),category='Gloves',type='gloves',skin=0,icon='sourceadvanced/gloves/'+directory.name,models=paths))
(root/'asset-audit.json').write_text(json.dumps(dict(catalog=catalog,bind_audit=bind_audit,materials=len(copied),missing=sorted(set(missing))),indent=2),encoding='utf8')
if missing:print('Missing dependencies:',sorted(set(missing)),flush=True);raise SystemExit(1)
if materials_only:print('CSSO materials collected for supported VertexLitGeneric shader.',flush=True);raise SystemExit(0)
exe=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\bin\studiomdl.exe')
def compile(qc):
 result=subprocess.run([str(exe),'-game',str(stage),'-nop4',str(qc)],capture_output=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW)
 (qc.parent/'compile.log').write_bytes(result.stdout+result.stderr)
 path='models/sourceadvanced/gloves/'+qc.parent.parent.name+'_'+qc.parent.name+'.mdl'
 ok=not result.returncode and (stage/path).is_file();print(qc.parent.parent.name,qc.parent.name,'PASS' if ok else 'FAIL',flush=True);return ok
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:results=list(pool.map(compile,qcs))
if not all(results):raise SystemExit(1)
print('Compiled',len(qcs),'CSSO glove rig models.',flush=True)
