import os
"""Stage original supplied firearm motion and CSSO gloves without gameplay scripts."""
from pathlib import Path
import hashlib,json,re,shutil
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'))
root=work/'animation-gloves-update';assets=root/'assets/cstrike'
base=work/'native-knife-engine-20260930'
firearms=work/'cs2-animation-import';gloves=work/'csso-glove-import'
for audit in (firearms/'asset-audit.json',gloves/'asset-audit.json'):
 if json.loads(audit.read_text())['missing']:raise RuntimeError('Missing dependencies: '+str(audit))
assets.mkdir(parents=True,exist_ok=True)
for source in (base/'assets/cstrike',firearms/'assets/cstrike',gloves/'assets/cstrike',work/'hud-pink/assets/cstrike',work/'cs2-hitbox-import/assets/cstrike',work/'cs2-audio-import/assets/cstrike'):
 for path in source.rglob('*'):
  if not path.is_file() or path.name=='gameinfo.txt':continue
  relative=path.relative_to(source)
  if relative.parts[0] not in ('models','materials','sound','scripts','resource'):continue
  dest=assets/relative;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,dest)
models={
 1000:'v_rif_ak47',1001:'v_rif_aug',1002:'v_snip_awp',1003:'v_pist_deagle',1005:'v_pist_elite',
 1006:'v_rif_famas',1008:'v_pist_fiveseven',1010:'v_snip_g3sg1',1011:'v_rif_galil',1012:'v_pist_glock18',
  # The Butterfly entry is compiled from the supplied CS2 motion rig.  It is
  # intentionally separate from the older native knife catalogue so the CS2
  # arm chain and blade pivots stay together.
  1015:'v_knife_t',
 1034:'v_mach_m249para',1036:'v_shot_m3super90',1038:'v_rif_m4a1',1040:'v_smg_mac10',1041:'v_smg_mp5',
 1043:'v_pist_p228',1044:'v_smg_p90',1046:'v_snip_scout',1047:'v_snip_sg550',1048:'v_rif_sg552',
 1049:'v_smg_tmp',1050:'v_smg_ump45',1052:'v_pist_usp',1054:'v_shot_xm1014'}
manifest=(base/'assets/cstrike/scripts/skins_manifest.txt').read_text()
catalog=[]
def update(match):
 item=int(match[1]);body=match[2]
 if item not in models:return match[0]
 path='models/sourceadvanced/cs2/'+models[item]+'.mdl'
 for suffix in ('.mdl','.vvd','.dx90.vtx'):
  if not (assets/Path(path).with_suffix(suffix)).is_file():raise RuntimeError('Uncompiled '+path+suffix)
 body=re.sub(r'("view_model"\s+)"[^"]+"',lambda m:m[1]+'"'+path+'"',body)
 body+='  "use_as_default" "1"\n '
 weapon=re.search(r'"weapon_class"\s+"([^"]+)"',body)[1]
 alias=weapon.removeprefix('weapon_')
 if alias=='mp5navy':alias='mp5navy'
 for field,event in [('shoot_sound','SACS2.Fire.'+alias+('_alt' if alias in ('m4a1','usp') else '')),('silenced_sound','SACS2.Fire.'+alias)]:
  body=re.sub(r'("'+field+r'"\s+)"[^"]+"',lambda m:m[1]+'"'+event+'"',body)
 name=re.search(r'"name"\s+"([^"]+)"',body)[1]
 catalog.append(dict(item_id=item,weapon=weapon,name=name,model=path))
 return '"'+str(item)+'"\n {'+body+'}'
manifest=re.sub(r'"(\d+)"\s*\{([^{}]+)\}',update,manifest)
assert len(catalog)==25
manifest=manifest.replace('{','{\n "animation_sound_script" "scripts/game_sounds_sourceadvanced_cs2.txt"',1)
glove_catalog=json.loads((gloves/'asset-audit.json').read_text())['catalog']
assert len(glove_catalog)==20 and next(g for g in glove_catalog if g['item_id']==4019)['name']=='Glove Sporty'
entries=[]
for glove in glove_catalog:
 fields={k:glove[k] for k in ('name','category','type','skin','icon')}
 fields.update({rig+'_model':path for rig,path in glove['models'].items()})
 entries.append(' "'+str(glove['item_id'])+'"\n {\n'+'\n'.join('  "'+k+'" "'+str(v)+'"' for k,v in fields.items())+'\n }')
manifest=manifest.rstrip().removesuffix('}')+'\n'+'\n'.join(entries)+'\n}\n'
(assets/'scripts/skins_manifest.txt').write_text(manifest,encoding='utf8')
runtime=root/'candidate-runtime'
shutil.copytree(base/'candidate-runtime',runtime,dirs_exist_ok=True,ignore=shutil.ignore_patterns('console.log','screenshots','*.mdmp'))
shutil.copytree(assets,runtime/'cstrike',dirs_exist_ok=True)
package=root/'000_sourceadvanced_catalog.vpk'
if package.is_file():shutil.copy2(package,runtime/'cstrike/custom/000_sourceadvanced_catalog.vpk')
repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
modules={}
for relative in ('cstrike/bin/client.dll','cstrike/bin/server.dll','bin/GameUI.dll'):
 source=repo/'output'/relative;dest=runtime/relative;shutil.copy2(source,dest)
 modules[relative]=hashlib.sha256(source.read_bytes()).hexdigest()
report=dict(firearms=catalog,gloves=glove_catalog,modules=modules,
 default_glove=4019,gameplay_scripts_imported=False,source_weapon_pack=str(firearms),source_gloves='CSSO',
 files={str(p.relative_to(assets)):hashlib.sha256(p.read_bytes()).hexdigest() for p in assets.rglob('*') if p.is_file()})
(root/'stage-audit.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('Staged 24 firearm models, 1 CS2 Butterfly model, and 20 CSSO glove types; inventory has 75 cards.',flush=True)
