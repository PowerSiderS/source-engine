import os
import argparse,json,re,secrets,subprocess,sys,time
from pathlib import Path
sys.path.insert(0,r'C:\Users\SnyX\Desktop\projeto clone\source-engine\tools')
from validate_native_instance import Rcon
parser=argparse.ArgumentParser();parser.add_argument('--video-only',action='store_true');parser.add_argument('--capture',action='store_true');parser.add_argument('--debug',action='store_true');args=parser.parse_args()
root=Path(os.environ.get('SA_KNIFE_WORK',r'C:\Users\SnyX\Documents\Codex\2026-09-28\c-users-snyx-desktop-pasta-teste\work'))/'native-knife-engine-20260930'
runtime=root/'candidate-runtime'; logs=root/('video-test' if args.video_only else 'candidate-test');logs.mkdir(exist_ok=True)
password=secrets.token_hex(16)
cfgname='native_test_'+secrets.token_hex(4)+'.cfg';runname='native_run_'+secrets.token_hex(4)+'.cfg'
cfg=runtime/'cstrike/cfg'/cfgname;run=runtime/'cstrike/cfg'/runname
cfg.write_text('host_competitive_ever_enabled 1\nsv_lan 1\nsv_cheats 1\nmp_autoteambalance 0\nmp_limitteams 0\nbot_quota 0\nmp_freezetime 0\nfps_max 60\nengine_no_focus_sleep 0\nsv_allow_wait_command 1\ndeveloper 1\nunbindall\nsensitivity 0\n',encoding='ascii')
manifest=(runtime/'cstrike/scripts/skins_manifest.txt').read_text()
items=[int(item) for item,body in re.findall(r'"(\d+)"\s*\{([^}]+)\}',manifest) if '"weapon_knife"' in body]
assert len(items)==21
commands=['alias sa_test_start "wait 180; cmd jointeam 2; wait 120; cmd joinclass 1; wait 120; setang -89 0 0; sa_test_0"']
for i,item in enumerate(items):
 commands.append(f'alias sa_test_{i} "inventory_equip {item}; use weapon_knife; wait 180; inspectlook; wait 30; '
                 'cl_inventory_validate; +attack; wait 90; cl_inventory_validate; -attack; wait 90; '
                 f'+attack2; wait 30; cl_inventory_validate; -attack2; wait 120; sa_test_{i+1}"')
commands.append(f'alias sa_test_{len(items)} "inventory_equip 1023; wait 180; inspectlook; wait 800; cl_inventory_validate; '
                'echo AIR_TEST_END; setang 89 0 0; +duck; wait 180; +attack; wait 90; cl_inventory_validate; '
                '-attack; -duck; wait 120; echo HIT_TEST_END; setang 0 0 0; give weapon_glock; inventory_equip 1012; '
                'use weapon_glock; wait 180; cl_inventory_validate; use weapon_knife; wait 180; cl_inventory_validate; '
                'mat_fast_video_changes 1; mat_profile_video_changes 1; sa_video_0"')
modes=[(1280,720,1),(800,600,1),(1280,720,0),(800,600,1)]
for i,(w,h,windowed) in enumerate(modes):
 nextcmd=f'sa_video_{i+1}' if i+1<len(modes) else 'echo NATIVE_TEST_DONE'
 commands.append(f'alias sa_video_{i} "mat_setvideomode {w} {h} {windowed}; wait 180; mat_configcurrent; cl_inventory_validate; {nextcmd}"')
commands.append('alias sa_test_start "wait 180; cmd jointeam 2; wait 120; cmd joinclass 1; wait 180; inventory_equip 1023; wait 180; mat_fast_video_changes 1; mat_profile_video_changes 1; sa_video_0"' if args.video_only else '')
if args.video_only:
 commands[-1]=commands[-1].replace('inventory_equip 1023; wait 180;','inventory_equip 1023; use weapon_knife; wait 180;')
if args.capture:
 commands=[c.replace('mat_fast_video_changes 1;', 'cl_screenshotname native_knife; screenshot; wait 60; mat_fast_video_changes 1;') for c in commands]
commands.append('sa_test_start');run.write_text('\n'.join(commands)+'\n',encoding='ascii')
private_log=runtime/'cstrike/console.log';private_log.unlink(missing_ok=True)
proc=subprocess.Popen([str(runtime/'hl2_launcher.exe'),'-game','cstrike','-multirun','-windowed','-w','800','-h','600','-novid','-condebug',
 '-usercon','-ip','127.0.0.1','-port','27682','-clientport','27683','+host_competitive_ever_enabled','1','+sv_lan','1','+rcon_password',password,
 '+servercfgfile',cfgname,'+lservercfgfile',cfgname,'+map','de_mirage_csgo_new'],cwd=runtime)
print('Private native test PID',proc.pid,flush=True)
debug_thread=None
if args.debug:
 from debug_native_crash import monitor
 debug_thread=monitor(proc.pid,logs)
start=time.monotonic();channel=None;ready=False;forced=False;ready_seconds=None;responses={}
try:
 while proc.poll() is None and time.monotonic()-start<180:
  try:
   channel=Rcon(27682,password);status=channel.command('status')
   if 'de_mirage_csgo_new' in status:ready=True;ready_seconds=round(time.monotonic()-start,2);break
  except (OSError,EOFError,RuntimeError):
   if channel:channel.sock.close();channel=None
  time.sleep(1)
 if ready:
  print('Map ready:',ready_seconds,'seconds',flush=True)
  responses['start']=channel.command('exec '+runname)
  deadline=time.monotonic()+360
  while proc.poll() is None and time.monotonic()<deadline:
   if private_log.exists() and 'NATIVE_TEST_DONE' in private_log.read_text(encoding='utf-8',errors='replace'):break
   time.sleep(1)
  if proc.poll() is None:
   responses['profile']=channel.command('cs_inventory_profile');channel.command('quit')
   try:proc.wait(timeout=20)
   except subprocess.TimeoutExpired:forced=True
 else:forced=True
finally:
 if channel:channel.sock.close()
 if proc.poll() is None:forced=True;proc.terminate();proc.wait(timeout=10)
 cfg.unlink(missing_ok=True);run.unlink(missing_ok=True)
text=private_log.read_text(encoding='utf-8',errors='replace') if private_log.exists() else ''
if debug_thread:debug_thread.join(timeout=5)
audits=[l for l in text.splitlines() if '[inventory-audit]' in l]
inspects=[l for l in text.splitlines() if '[inspect-audit]' in l]
attacks=[l for l in text.splitlines() if '[knife-animation]' in l]
videos=[l for l in text.splitlines() if '[video-profile]' in l]
seen={int(re.search(r'item=(\d+)',l)[1]) for l in audits if 'sequence=lookat' in l and 'looking=1' in l}
valid=bool(audits) and all('missing_bones=0 invalid_matrices=0' in l and
 ('arms=models/sourceadvanced/c_arms_native.mdl' in l if 'c_weapon_knife_' in l else 'arms=models/sourceadvanced/c_arms_default.mdl' in l) for l in audits)
air_attacks=[l for l in text.split('AIR_TEST_END')[0].splitlines() if '[knife-animation]' in l]
contact_attacks=[l for l in text.split('AIR_TEST_END')[-1].split('HIT_TEST_END')[0].splitlines() if '[knife-animation]' in l]
misses=bool(air_attacks) and all('hit=0' in l and ('sequence=light_miss' in l or 'sequence=heavy_miss' in l) for l in air_attacks)
hits=any('light hit=1 sequence=light_hit' in l for l in contact_attacks)
arms_switch=any('glock' in l and 'arms=models/sourceadvanced/c_arms_default.mdl' in l for l in audits)
correct_modes=re.findall(r'width: (\d+)\s+height: (\d+)',text)==[(str(w),str(h)) for w,h,_ in modes]
fast= len(videos)==4 and all('path=d3d9ex retained_textures=1' in l for l in videos)
idle=any('m9_bayonet' in l and 'sequence=idle' in l and 'looking=0' in l for l in audits)
result=dict(pid=proc.pid,ready=ready,ready_seconds=ready_seconds,elapsed=round(time.monotonic()-start,2),exit=proc.returncode,forced_shutdown=forced,
 completed='NATIVE_TEST_DONE' in text,all_21_native_inspections=seen==set(items),valid_arm_bones=valid,air_attacks_use_miss_only=misses,
 inspection_returns_idle=idle,contact_uses_hit_animation=hits,gun_knife_arm_switch=arms_switch,correct_video_modes=correct_modes,fast_video=fast,visual_deformation_reviewed=False,
 audits=audits,inspections=inspects,attacks=attacks,video_profiles=videos)
result['passed']=bool(ready and proc.returncode==0 and not forced and result['completed'] and valid and correct_modes and fast and (args.video_only or seen==set(items) and misses and idle and hits and arms_switch))
(logs/'console.log').write_text(text,encoding='utf-8');(logs/'rcon.json').write_text(json.dumps(responses,indent=2),encoding='utf-8')
(logs/'result.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k not in ('audits','inspections','attacks')},indent=2),flush=True)
raise SystemExit(0 if result['passed'] else 1)
