"""Measure map transitions until the client's actual loading screen completes."""
from pathlib import Path
import json,re,secrets,shutil,subprocess,sys,time
sys.path.insert(0,r'C:\Users\SnyX\Desktop\projeto clone\source-engine\tools')
from validate_native_instance import Rcon
root=Path(__file__).parent/'animation-gloves-update'
runtime=root/'timing-runtime';logs=root/'map-loading-profile';logs.mkdir(exist_ok=True)
shutil.copytree(root/'candidate-runtime',runtime,dirs_exist_ok=True,ignore=shutil.ignore_patterns('console.log','screenshots','*.mdmp'))
maps=[l.strip() for l in (runtime/'cstrike/cfg/sourceadvanced_maps.txt').read_text().splitlines() if l.strip() and not l.startswith('#')]
password=secrets.token_hex(16);port=27690
cfgname='timing_'+secrets.token_hex(4)+'.cfg';cfg=runtime/'cstrike/cfg'/cfgname
cfg.write_text('host_competitive_ever_enabled 1\nsv_lan 1\nsv_cheats 1\nbot_quota 0\nfps_max 60\nengine_no_focus_sleep 0\ncl_profile_map_load 1\nunbindall\nsensitivity 0\n',encoding='ascii')
log=runtime/'cstrike/console.log';log.unlink(missing_ok=True)
started=time.monotonic()
proc=subprocess.Popen([str(runtime/'hl2_launcher.exe'),'-game','cstrike','-multirun','-windowed','-w','800','-h','600','-novid','-condebug',
 '-usercon','-ip','127.0.0.1','-port',str(port),'-clientport','27691','+host_competitive_ever_enabled','1','+sv_lan','1','+rcon_password',password,
 '+cl_profile_map_load','1','+servercfgfile',cfgname,'+lservercfgfile',cfgname,'+map','de_mirage_csgo_new'],cwd=runtime)
print('Private loading profile PID',proc.pid,flush=True)
channel=None;forced=False;records=[];bootstrap=None
def wait_active(mapname,start,offset=0):
 global channel
 server=None;last=''
 while proc.poll() is None and time.monotonic()-start<600:
  try:
   channel=Rcon(port,password);channel.sock.settimeout(.2)
   last=channel.command('status')
   if re.search(r'^map\s*:\s*'+re.escape(mapname)+r'\b',last,re.M):
    if server is None:server=round(time.monotonic()-start,2)
    if log.exists():
     with log.open('rb') as stream:stream.seek(offset);segment=stream.read().decode('utf8','replace')
    else:segment=''
    marker=re.search(r'\[load-profile\] event=client_ready map='+re.escape(mapname)+r' t=([\d.]+)',segment)
    if marker and re.search(r'^#\s+\d+\s+"[^\n]+\bactive\b',last,re.M):
     phases=[dict(phase=p,milliseconds=float(ms)) for p,ms in re.findall(r'\[load-profile\] event=end phase=(\w+) map='+re.escape(mapname)+r' ms=([\d.]+)',segment)]
     return dict(map=mapname,server_ready_seconds=server,client_active_seconds=round(time.monotonic()-start,2),client_ready_engine_timestamp=float(marker[1]),client_ready_marker=True,phases=phases,status=last,passed=True)
   channel.sock.close();channel=None
  except (OSError,EOFError,RuntimeError):
   if channel:channel.sock.close();channel=None
  time.sleep(.5)
 return dict(map=mapname,server_ready_seconds=server,client_active_seconds=None,status=last,passed=False)
try:
 bootstrap=wait_active('de_mirage_csgo_new',started)
 if not bootstrap['passed']:raise RuntimeError('Bootstrap client did not become active')
 print('Initial executable startup to Mirage active:',bootstrap['client_active_seconds'],'s',flush=True)
 for mapname in maps:
  offset=log.stat().st_size;start=time.monotonic();channel.command('changelevel '+mapname);channel.sock.close();channel=None
  record=wait_active(mapname,start,offset);records.append(record)
  (logs/'progress.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
  print(mapname,'server',record['server_ready_seconds'],'client active',record['client_active_seconds'],'PASS' if record['passed'] else 'FAIL',flush=True)
  if not record['passed']:break
 if proc.poll() is None and channel:
  channel.sock.settimeout(2);channel.command('quit')
  try:proc.wait(timeout=20)
  except subprocess.TimeoutExpired:forced=True
finally:
 if channel:channel.sock.close()
 if proc.poll() is None:forced=True;proc.terminate();proc.wait(timeout=10)
 cfg.unlink(missing_ok=True)
report=dict(pid=proc.pid,bootstrap=bootstrap,maps=records,exit=proc.returncode,forced_shutdown=forced,
 elapsed=round(time.monotonic()-started,2),conditions='Private native client, windowed 800x600, fps_max 60, no bots; cache warm after previous all-map tests. Each measured map is a changelevel from an already active session. Completion requires client_ready emitted after CL_FullyConnected closes the loading screen, plus server status active. cl_profile_map_load enabled. Polling approximately 0.5-1 second. No disk or shader caches were flushed.',
 passed=len(records)==10 and all(r['passed'] for r in records) and proc.returncode==0 and not forced)
(logs/'result.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('Loading profile passed:',report['passed'],flush=True)
raise SystemExit(0 if report['passed'] else 1)
