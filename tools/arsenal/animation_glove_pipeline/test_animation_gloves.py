import os
"""Native gameplay/rig integration tests in an isolated runtime."""
from pathlib import Path
import argparse,hashlib,json,re,secrets,shutil,subprocess,sys,time
parser=argparse.ArgumentParser();parser.add_argument('--quick',action='store_true');parser.add_argument('--wrist',action='store_true');parser.add_argument('--visual-guns',action='store_true');parser.add_argument('--knife',action='store_true',help='Validate the imported CS2 Butterfly sequence set in the private runtime.');args=parser.parse_args()
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));root=work/'animation-gloves-update';runtime=root/'candidate-runtime'
logs=root/('visual-guns-test' if args.visual_guns else 'wrist-test' if args.wrist else 'quick-test' if args.quick else 'native-test');logs.mkdir(exist_ok=True)
sys.path.insert(0,r'C:\Users\SnyX\Desktop\projeto clone\source-engine\tools')
from validate_native_instance import Rcon
from debug_native_crash import monitor
audit=json.loads((root/'stage-audit.json').read_text())
manifest=runtime/'cstrike/scripts/skins_manifest.txt'
manifest_backup=None
# The full cosmetic catalogue contains older world-model ports that are
# intentionally outside this focused test.  Source 1 validates the bounds of
# every server-precached model during signon, so keep the private Butterfly
# run limited to its view/world pair.  The user's runtime manifest is never
# changed and the candidate manifest is restored in finally.
if args.knife:
 manifest_backup=logs/'skins_manifest.before-knife-test.txt'
 shutil.copy2(manifest,manifest_backup)
 manifest.write_text('''"SkinsManifest"
{
 "animation_sound_script" "scripts/game_sounds_sourceadvanced_cs2.txt"
 "1015"
 {
  "name" "Butterfly Knife"
  "category" "Facas"
  "weapon_class" "weapon_knife"
  "view_model" "models/sourceadvanced/cs2/v_knife_t.mdl"
  "world_model" "models/sourceadvanced/w_weapon_knife_butterfly_knife.mdl"
  "skin" "0"
 }
}
''',encoding='utf8')
cfgname='anim_test_'+secrets.token_hex(4)+'.cfg';runname='anim_run_'+secrets.token_hex(4)+'.cfg'
cfg=runtime/'cstrike/cfg'/cfgname;run=runtime/'cstrike/cfg'/runname
cfg.write_text('host_competitive_ever_enabled 1\nsv_lan 1\nsv_cheats 1\nmp_autoteambalance 0\nmp_limitteams 0\nbot_quota 0\nmp_freezetime 0\nmp_roundtime 60\nfps_max 60\nengine_no_focus_sleep 0\nsv_allow_wait_command 1\ndeveloper 1\ncl_profile_map_load 1\ncl_disablehtmlmotd 1\nunbindall\nsensitivity 0\ncl_inventory_loadout ""\ninventory_gloves 0\n',encoding='ascii')
commands=['alias sa_test_start "wait 180; cmd jointeam 1; wait 90; fps_max 60; cmd jointeam 2; wait 120; cmd joinclass 1; wait 180; hidepanel info; hidepanel team; hidepanel class_ter; hidepanel class_ct; gameui_hide; firstperson; noclip; setang -30 0 0; sa_gun_0"']
clear_weapons='; '.join('ent_remove_all '+w for w in sorted({item['weapon'] for item in audit['firearms']}|{'weapon_knife'}))+'; wait 60; '
for i,item in enumerate(audit['firearms']):
 id=item['item_id'];weapon=item['weapon'];nextcmd=f'sa_gun_{i+1}' if i<23 and not args.quick else 'sa_glove_start'
 if args.visual_guns and i==23:nextcmd='sa_visual_done'
 def cap(step):return f'cl_screenshotname gun_{id}_{step}; screenshot; wait 10; ' if args.visual_guns or id in (1046,1034,1047,1050,1052) else ''
 shot='glock_fire' if id==1012 else 'shoot'
 commands.append(f'alias sa_gun_{i} "{clear_weapons}inventory_equip {id}; give {weapon}; wait 60; use {weapon}; wait 180; '
  f'echo FIREARM_{id}_IDLE; cl_inventory_validate; {cap("idle")}inventory_unequip; wait 30; echo FIREARM_{id}_DEFAULT; cl_inventory_validate; '
  f'inspectlook; wait 60; echo FIREARM_{id}_INSPECT; cl_inventory_validate; {cap("inspect")}+attack; wait 4; echo FIREARM_{id}_SHOT; cl_inventory_validate; {cap("fire")}'
  f'-attack; wait 300; +reload; wait 60; echo FIREARM_{id}_RELOAD; cl_inventory_validate; {cap("reload")}-reload; wait 600; '
  f'echo FIREARM_{id}_END; cl_inventory_validate; {nextcmd}"')
commands.append('alias sa_glove_start "'+clear_weapons+'give weapon_ak47; wait 60; use weapon_ak47; inventory_equip 1000; wait 120; '+('sa_glove_19' if args.quick else 'sa_glove_0')+'"')
glove_pairs=[]
for rig,item,weapon in [('cs2',1000,'weapon_ak47'),('native',1022,'weapon_knife'),('legacy',1039,'weapon_m4a1')]:
 for glove in audit['gloves']:glove_pairs.append((rig,item,weapon,glove['item_id']))
for i,(rig,item,weapon,glove) in enumerate(glove_pairs):
 nextcmd=f'sa_glove_{i+1}' if i<len(glove_pairs)-1 else 'sa_final'
 if args.quick:nextcmd={19:'sa_glove_39',39:'sa_glove_59',59:'sa_final'}.get(i,nextcmd)
 setup=''
 if i in ((39,59) if args.quick else (20,40)):
  setup=clear_weapons+f'give {weapon}; wait 60; inventory_equip {item}; use {weapon}; wait 120; '
 capture=f'cl_screenshotname csso_{rig}_{glove}_idle; screenshot; wait 10; ' if glove in (4000,4002,4019) else ''
 capture_inspect=f'cl_screenshotname csso_{rig}_{glove}_inspect; screenshot; wait 10; ' if glove in (4000,4002,4019) else ''
 commands.append(f'alias sa_glove_{i} "{setup}inventory_gloves {glove}; wait 60; echo GLOVE_{rig}_{glove}_IDLE; cl_inventory_validate; '
  f'{capture}inspectlook; wait 30; echo GLOVE_{rig}_{glove}_INSPECT; cl_inventory_validate; {capture_inspect}'
  f'+attack; wait 4; -attack; wait 120; {nextcmd}"')
commands+=['alias sa_final "inventory_gloves 0; give weapon_knife; wait 60; use weapon_knife; inventory_equip 1023; wait 90; inspectlook; wait 30; '
 'echo DEFAULT_CSSO_GLOVE; cl_inventory_validate; cl_screenshotname csso_default_m9; screenshot; wait 10; +attack; wait 4; -attack; wait 90; '
 'mat_profile_video_changes 1; mat_setvideomode 1024 768 1; wait 180; cl_inventory_validate; '
 'mat_setvideomode 800 600 1; wait 180; cl_inventory_validate; open_inventory; wait 180; cl_screenshotname inventory_csgo_final; screenshot; wait 30; gameui_hide; echo ANIMATION_GLOVES_DONE"','sa_test_start']
if args.wrist:
 commands=[
 'alias sa_test_start "wait 180; cmd jointeam 1; wait 90; fps_max 60; cmd jointeam 2; wait 120; cmd joinclass 1; wait 180; hidepanel info; hidepanel team; hidepanel class_ter; hidepanel class_ct; gameui_hide; firstperson; noclip; setang -30 0 0; ent_remove_all weapon_*; wait 30; give weapon_ak47; wait 30; use weapon_ak47; inventory_equip 1000; inventory_gloves 4019; wait 180; sa_wrist_idle"',
 'alias sa_wrist_idle "echo WRIST_IDLE; cl_inventory_validate; cl_screenshotname wrist_fixed_idle; screenshot; wait 30; +attack; wait 4; echo WRIST_FIRE; cl_inventory_validate; cl_screenshotname wrist_fixed_fire; screenshot; -attack; wait 180; sa_wrist_reload"',
 'alias sa_wrist_reload "+reload; wait 60; echo WRIST_RELOAD; cl_inventory_validate; cl_screenshotname wrist_fixed_reload; screenshot; -reload; wait 600; inspectlook; wait 60; echo WRIST_INSPECT; cl_inventory_validate; cl_screenshotname wrist_fixed_inspect; screenshot; wait 60; echo ANIMATION_GLOVES_DONE"',
 'sa_test_start']
if args.knife:
 commands=[
 'alias sa_test_start "wait 180; cmd jointeam 1; wait 90; fps_max 60; cmd jointeam 2; wait 120; cmd joinclass 1; wait 180; hidepanel info; hidepanel team; hidepanel class_ter; hidepanel class_ct; gameui_hide; firstperson; noclip; ent_remove_all weapon_*; wait 60; give weapon_knife; wait 60; inventory_equip 1015; use weapon_knife; wait 180; echo KNIFE_IDLE; cl_inventory_validate; inspectlook; wait 180; echo KNIFE_INSPECT; cl_inventory_validate; +attack; wait 4; -attack; wait 180; echo KNIFE_LIGHT; cl_inventory_validate; +attack2; wait 4; -attack2; wait 180; echo KNIFE_HEAVY; cl_inventory_validate; echo ANIMATION_GLOVES_DONE"',
 'sa_test_start']
if args.visual_guns:commands.insert(-1,'alias sa_visual_done "echo ANIMATION_GLOVES_DONE"')
def split_alias(definition):
 match=re.fullmatch(r'alias (\w+) "([^"]*)"',definition)
 if not match:return [definition]
 name,body=match.groups();chunks=[];chunk=[]
 for command in body.split(';'):
  command=command.strip()
  if len('; '.join(chunk+[command]))>360 and chunk:chunks.append(chunk);chunk=[]
  chunk.append(command)
 if chunk:chunks.append(chunk)
 result=[]
 for i,chunk in enumerate(chunks):
  alias=name if i==0 else name+'_p'+str(i)
  if i+1<len(chunks):chunk.append(name+'_p'+str(i+1))
  result.append('alias '+alias+' "'+'; '.join(chunk)+'"')
 return result
commands=[line for command in commands for line in split_alias(command)]
assert all(len(line)<512 for line in commands)
run.write_text('\n'.join(commands)+'\n',encoding='ascii')
(logs/'test.cfg').write_text(run.read_text(),encoding='ascii')
log=runtime/'cstrike/console.log';log.unlink(missing_ok=True)
password=secrets.token_hex(16);port=27686
proc=subprocess.Popen([str(runtime/'hl2_launcher.exe'),'-game','cstrike','-multirun','-windowed','-w','800','-h','600','-novid','-condebug','-window_name_suffix','TESTE PRIVADO - ANIMACOES E LUVAS',
 '-usercon','-ip','127.0.0.1','-port',str(port),'-clientport','27687','+host_competitive_ever_enabled','1','+sv_lan','1','+rcon_password',password,
 '+fps_max','60','+servercfgfile',cfgname,'+lservercfgfile',cfgname,'+map','de_mirage_csgo_new'],cwd=runtime)
print('Private animation/glove test PID',proc.pid,flush=True)
debug=monitor(proc.pid,logs);start=time.monotonic();channel=None;forced=False;ready=False;responses={};nuke=False;error=None
def tail():
 if not log.exists():return ''
 with log.open('rb') as stream:stream.seek(max(0,log.stat().st_size-250000));return stream.read().decode('utf8','replace').replace('\r','')
def connect_ready(mapname,timeout):
 global channel
 deadline=time.monotonic()+timeout
 while proc.poll() is None and time.monotonic()<deadline:
  try:
   channel=Rcon(port,password);status=channel.command('status')
   if mapname in status and re.search(r'^#\s+\d+\s+"[^\n]+\bactive\b',status,re.M):return True
   channel.sock.close();channel=None
  except (OSError,EOFError,RuntimeError):
   if channel:channel.sock.close();channel=None
  time.sleep(1)
 return False
try:
 ready=connect_ready('de_mirage_csgo_new',360)
 if not ready:raise RuntimeError('Mirage did not become active')
 print('Mirage active after',round(time.monotonic()-start,1),'seconds.',flush=True)
 responses['start']=channel.command('exec '+runname)
 (logs/'rcon-start.txt').write_text(responses['start'],encoding='utf8')
 deadline=time.monotonic()+1200;last_marker=''
 while proc.poll() is None and time.monotonic()<deadline:
  text=tail();markers=re.findall(r'^(FIREARM_\d+_\w+|GLOVE_\w+_\d+_\w+)\s*$',text,re.M)
  if markers and markers[-1]!=last_marker:last_marker=markers[-1];print(last_marker,flush=True)
  if 'ANIMATION_GLOVES_DONE' in text:break
  time.sleep(2)
 if 'ANIMATION_GLOVES_DONE' not in tail():raise RuntimeError('Integration sequence did not finish')
 responses['profile']=channel.command('cs_inventory_profile')
 channel.command('changelevel de_nuke_csgo_new' if not (args.quick or args.wrist or args.visual_guns or args.knife) else 'echo QUICK_CAPTURE_DONE');channel.sock.close();channel=None
 nuke=connect_ready('de_nuke_csgo_new' if not (args.quick or args.wrist or args.visual_guns or args.knife) else 'de_mirage_csgo_new',420)
 if not nuke:raise RuntimeError('Nuke did not become active')
 commands2='cmd jointeam 2; wait 120; cmd joinclass 1; wait 180; give weapon_ak47; inventory_equip 1000; use weapon_ak47; wait 120; inventory_gloves 4019; wait 60; cl_inventory_validate; use weapon_knife; inventory_equip 1022; wait 120; cl_inventory_validate; echo NUKE_GLOVES_DONE'
 ncfg=runtime/'cstrike/cfg'/('nuke_'+secrets.token_hex(4)+'.cfg');ncfg.write_text(commands2+'\n',encoding='ascii')
 channel.command('exec '+ncfg.name)
 deadline=time.monotonic()+50
 while proc.poll() is None and time.monotonic()<deadline and 'NUKE_GLOVES_DONE' not in tail():time.sleep(1)
 ncfg.unlink(missing_ok=True)
 channel.command('quit')
 try:proc.wait(timeout=30)
 except subprocess.TimeoutExpired:forced=True
except Exception as exc:
 error=str(exc);print('Native test error:',error,flush=True)
finally:
 if channel:channel.sock.close()
 if proc.poll() is None:forced=True;proc.terminate();proc.wait(timeout=10)
 cfg.unlink(missing_ok=True);run.unlink(missing_ok=True)
 if manifest_backup and manifest_backup.is_file():shutil.copy2(manifest_backup,manifest)
 debug.join(timeout=5)
text=log.read_text(encoding='utf8',errors='replace') if log.exists() else ''
lines=text.splitlines();audits=[line for line in lines if '[inventory-audit]' in line]
tagged={}
for i,line in enumerate(lines):
 line=line.strip()
 if re.fullmatch(r'(FIREARM_\d+_\w+|GLOVE_\w+_\d+_\w+|WRIST_\w+|DEFAULT_CSSO_GLOVE)',line):
  match=next((nextline for nextline in lines[i+1:i+20] if '[inventory-audit]' in nextline),None)
  if match:tagged[line]=match
def valid(line):
 counts=re.search(r'embedded=(\d+) hidden=(\d+)',line)
 return 'missing_bones=0 invalid_matrices=0' in line and 'arms=models/sourceadvanced/gloves/' in line and counts and counts[1]==counts[2]
firearm_results=[]
for item in audit['firearms']:
 id=item['item_id'];records={step:tagged.get(f'FIREARM_{id}_{step}','') for step in ('IDLE','DEFAULT','INSPECT','SHOT','RELOAD','END')}
 success=all(valid(line) and 'model='+item['model']+' ' in line for line in records.values())
 success=success and 'item=0 ' in records['DEFAULT'] and 'sequence=lookat' in records['INSPECT'] and 'looking=1' in records['INSPECT']
 success=success and re.search(r'sequence=\S*(shoot|fire)',records['SHOT']) and re.search(r'sequence=\S*reload',records['RELOAD'])
 if id==1002:success=success and 'clip=5/5' in records['IDLE']
 firearm_results.append(dict(item=id,model=item['model'],records=records,passed=bool(success)))
glove_results=[]
for rig,item,weapon,glove in glove_pairs:
 records={step:tagged.get(f'GLOVE_{rig}_{glove}_{step}','') for step in ('IDLE','INSPECT')}
 model=next(g for g in audit['gloves'] if g['item_id']==glove)['models'][rig]
 success=all(valid(line) and 'arms='+model+' ' in line and f'glove={glove} ' in line for line in records.values())
 glove_results.append(dict(rig=rig,glove=glove,records=records,passed=bool(success)))
report=dict(pid=proc.pid,error=error,ready=ready,nuke_ready=nuke,nuke_completed='NUKE_GLOVES_DONE' in text,exit=proc.returncode,forced_shutdown=forced,
 elapsed=round(time.monotonic()-start,2),firearms=firearm_results,gloves=glove_results,all_audits_valid=bool(audits) and all(valid(a) for a in audits),
 default_csso_glove='glove=4019 ' in tagged.get('DEFAULT_CSSO_GLOVE',''),completed='ANIMATION_GLOVES_DONE' in text,
 modules={r:hashlib.sha256((runtime/r).read_bytes()).hexdigest() for r in audit['modules']},
 captures=[str(p.relative_to(runtime)) for p in (runtime/'cstrike/screenshots').glob('csso_*')],visual_review_complete=False,responses=responses)
report['imported_material_errors']=[line for line in lines if 'sourceadvanced/' in line and ('uses unknown shader' in line or 'proxy ' in line and 'not found' in line or 'KeyValues Error' in line)]
report['passed']=bool(ready and nuke and report['nuke_completed'] and report['completed'] and report['default_csso_glove'] and report['all_audits_valid'] and
 proc.returncode==0 and not forced and not report['imported_material_errors'] and all(r['passed'] for r in firearm_results+glove_results))
if args.quick:
 report['passed']=bool(ready and report['completed'] and proc.returncode==0 and not forced and not report['imported_material_errors'] and firearm_results[0]['passed'] and all(glove_results[i]['passed'] for i in (19,39,59)))
if args.wrist:
 report['wrist_records']={step:tagged.get('WRIST_'+step,'') for step in ('IDLE','FIRE','RELOAD','INSPECT')}
 report['passed']=bool(ready and report['completed'] and proc.returncode==0 and not forced and not report['imported_material_errors'] and all(valid(line) for line in report['wrist_records'].values()))
if args.visual_guns:
 report['passed']=bool(ready and report['completed'] and proc.returncode==0 and not forced and not report['imported_material_errors'] and all(r['passed'] for r in firearm_results))
if args.knife:
 report['knife_records']={step:tagged.get('KNIFE_'+step,'') for step in ('IDLE','INSPECT','LIGHT','HEAVY')}
 knife_model='model=models/sourceadvanced/cs2/v_knife_t.mdl '
 records=report['knife_records']
 report['passed']=bool(ready and report['completed'] and proc.returncode==0 and not forced and not report['imported_material_errors'] and
  all(valid(line) and knife_model in line for line in records.values()) and
  'sequence=lookat' in records['INSPECT'] and 'sequence=light_' in records['LIGHT'] and 'sequence=heavy_' in records['HEAVY'])
(logs/'result.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('Firearms',sum(r['passed'] for r in firearm_results),'/24; glove/rig pairs',sum(r['passed'] for r in glove_results),'/60; passed',report['passed'],flush=True)
raise SystemExit(0 if report['passed'] else 1)
