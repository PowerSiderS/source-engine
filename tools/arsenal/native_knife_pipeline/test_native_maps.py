import os
"""Test native map loading, knife use and video modes across the full catalog."""
from pathlib import Path
import hashlib,json,re,secrets,shutil,subprocess,sys,time
sys.path.insert(0,r'C:\Users\SnyX\Desktop\projeto clone\source-engine\tools')
from validate_native_instance import Rcon
from debug_native_crash import monitor
root=Path(os.environ.get('SA_KNIFE_WORK',r'C:\Users\SnyX\Documents\Codex\2026-09-28\c-users-snyx-desktop-pasta-teste\work'))/'native-knife-engine-20260930'
runtime=root/'maps-runtime';logs=root/'all-maps-test';logs.mkdir(exist_ok=True)
shutil.copytree(root/'candidate-runtime',runtime,dirs_exist_ok=True,ignore=shutil.ignore_patterns('console.log','screenshots','*.mdmp'))
maps=[l.strip() for l in (runtime/'cstrike/cfg/sourceadvanced_maps.txt').read_text().splitlines() if l.strip() and not l.startswith('#')]
assert len(maps)==10
if '--map' in sys.argv:
 maps=[sys.argv[sys.argv.index('--map')+1]]
 logs=root/('focused-map-test-'+maps[0]);logs.mkdir(exist_ok=True)
password=secrets.token_hex(16);port=27686
cfgname='maps_test_'+secrets.token_hex(4)+'.cfg';runname='maps_run_'+secrets.token_hex(4)+'.cfg'
cfg=runtime/'cstrike/cfg'/cfgname;run=runtime/'cstrike/cfg'/runname
cfg.write_text('host_competitive_ever_enabled 1\nsv_lan 1\nsv_cheats 1\nmp_autoteambalance 0\nmp_limitteams 0\nbot_quota 0\nmp_freezetime 0\nfps_max 60\nengine_no_focus_sleep 0\nsv_allow_wait_command 1\ndeveloper 1\nunbindall\nsensitivity 0\n',encoding='ascii')
log=runtime/'cstrike/console.log';log.unlink(missing_ok=True)
proc=subprocess.Popen([str(runtime/'hl2_launcher.exe'),'-game','cstrike','-multirun','-windowed','-w','800','-h','600','-novid','-condebug',
 '-usercon','-ip','127.0.0.1','-port',str(port),'-clientport','27687','+host_competitive_ever_enabled','1','+sv_lan','1','+rcon_password',password,
 '+servercfgfile',cfgname,'+lservercfgfile',cfgname,'+map',maps[0]],cwd=runtime)
debug=monitor(proc.pid,logs);print('All-map private test PID',proc.pid,flush=True)
start=time.monotonic();results=[];forced=False;channel=None
try:
 for number,mapname in enumerate(maps):
  mapstart=time.monotonic();before=log.stat().st_size if log.exists() else 0
  if number:
   channel.command('changelevel '+mapname);channel.sock.close();channel=None
  ready=False
  while proc.poll() is None and time.monotonic()-mapstart<240:
   try:
    channel=Rcon(port,password);status=channel.command('status')
    if re.search(r'^map\s*:\s*'+re.escape(mapname)+r'\b',status,re.M):ready=True;break
    channel.sock.close();channel=None
   except (OSError,EOFError,RuntimeError):
    if channel:channel.sock.close();channel=None
   time.sleep(1)
  loadtime=round(time.monotonic()-mapstart,2)
  if not ready:results.append(dict(map=mapname,ready=False,passed=False));break
  token='MAP_TEST_DONE_'+str(number)
  commands=['alias sa_map_start "wait 180; cmd jointeam 2; wait 90; cmd joinclass 1; wait 180; setang -89 0 0; sa_map_0"']
  for i,item in enumerate((1015,1022,1023)):
   commands.append(f'alias sa_map_{i} "inventory_equip {item}; use weapon_knife; wait 180; inspectlook; wait 30; cl_inventory_validate; '
                   '+attack; wait 90; cl_inventory_validate; -attack; wait 90; +attack2; wait 30; cl_inventory_validate; '
                   f'-attack2; wait 120; sa_map_{i+1}"')
  commands.append('alias sa_map_3 "give weapon_glock; inventory_equip 1012; use weapon_glock; wait 180; cl_inventory_validate; '
                  'use weapon_knife; wait 180; cl_inventory_validate; mat_fast_video_changes 1; mat_profile_video_changes 1; sa_map_video_0"')
  modes=[(1280,720,1),(800,600,1),(1280,720,0),(800,600,1)]
  for i,(w,h,windowed) in enumerate(modes):
   nextcmd=f'sa_map_video_{i+1}' if i<3 else 'echo '+token
   commands.append(f'alias sa_map_video_{i} "mat_setvideomode {w} {h} {windowed}; wait 180; mat_configcurrent; cl_inventory_validate; {nextcmd}"')
  commands.append('sa_map_start');run.write_text('\n'.join(commands)+'\n',encoding='ascii')
  channel.command('exec '+runname)
  deadline=time.monotonic()+120
  while proc.poll() is None and time.monotonic()<deadline:
   segment=log.read_bytes()[before:].decode('utf-8','replace') if log.exists() else ''
   if token in segment:break
   time.sleep(1)
  segment=log.read_bytes()[before:].decode('utf-8','replace')
  audits=[l for l in segment.splitlines() if '[inventory-audit]' in l]
  inspects=[l for l in segment.splitlines() if '[inspect-audit]' in l]
  attacks=[l for l in segment.splitlines() if '[knife-animation]' in l]
  video=[l for l in segment.splitlines() if '[video-profile]' in l]
  seen={int(re.search(r'item=(\d+)',l)[1]) for l in audits if 'sequence=lookat' in l and 'looking=1' in l}
  valid=bool(audits) and all('missing_bones=0 invalid_matrices=0' in l and ('c_arms_native.mdl' in l if 'c_weapon_knife_' in l else 'c_arms_default.mdl' in l) for l in audits)
  misses=bool(attacks) and all('hit=0' in l and ('sequence=light_miss' in l or 'sequence=heavy_miss' in l) for l in attacks)
  balance=channel.command('cs_validate_weapon_balance') if proc.poll() is None else ''
  profile=channel.command('cs_inventory_profile') if proc.poll() is None else ''
  props=channel.command('staticprop_validate_handles') if proc.poll() is None else ''
  vertex_errors={}
  for name in re.findall(r"Error Vertex File for '([^']+)'",segment):vertex_errors[name]=vertex_errors.get(name,0)+1
  result=dict(map=mapname,ready=True,ready_seconds=loadtime,completed=token in segment,inspected=sorted(seen),valid_arm_bones=valid,
              air_attacks_use_miss_only=misses,audits=audits,inspections=inspects,video_profiles=video,inventory_profile=profile,
              balance=balance,staticprop_audit=props,vertex_error_counts=vertex_errors,elapsed=round(time.monotonic()-mapstart,2))
  result['passed']=bool(proc.poll() is None and result['completed'] and seen=={1015,1022,1023} and valid and misses and
                        len(video)==4 and all('path=d3d9ex retained_textures=1' in l for l in video) and '[balance-audit] checked=34 failed=0' in balance and '[staticprop-audit]' in props and 'failures=0' in props)
  (logs/(mapname+'.log')).write_text(segment,encoding='utf-8');results.append(result)
  (logs/'progress.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
  print(mapname,'PASS' if result['passed'] else 'FAIL','ready',loadtime,'s','video_ms',[re.search(r'total_ms=([\d.]+)',l)[1] for l in video],flush=True)
  if not result['passed']:break
 if proc.poll() is None and channel:
  channel.command('quit')
  try:proc.wait(timeout=20)
  except subprocess.TimeoutExpired:forced=True
finally:
 if channel:channel.sock.close()
 if proc.poll() is None:forced=True;proc.terminate();proc.wait(timeout=10)
 debug.join(timeout=5);cfg.unlink(missing_ok=True);run.unlink(missing_ok=True)
report=dict(pid=proc.pid,exit=proc.returncode,forced_shutdown=forced,elapsed=round(time.monotonic()-start,2),maps=results,
            modules={str(p.relative_to(runtime)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [runtime/'bin/engine.dll',runtime/'bin/datacache.dll',runtime/'cstrike/bin/client.dll',runtime/'cstrike/bin/server.dll']},
            passed=len(results)==len(maps) and all(r['passed'] for r in results) and proc.returncode==0 and not forced)
(logs/'result.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('All maps:',len(results),'exit',proc.returncode,'passed',report['passed'],flush=True)
raise SystemExit(0 if report['passed'] else 1)
