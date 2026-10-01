"""Private dedicated plus a separate real client; measure actual loading completion."""
from pathlib import Path
import json,re,secrets,shutil,subprocess,sys,time
sys.path.insert(0,r'C:\Users\SnyX\Desktop\projeto clone\source-engine\tools')
from validate_native_instance import Rcon
root=Path(__file__).parent/'animation-gloves-update'
logs=root/'dedicated-loading-profile';logs.mkdir(exist_ok=True)
server=root/'connection-server-runtime';client=root/'connection-client-runtime'
for source,dest in ((root/'candidate-dedicated-runtime',server),(root/'candidate-runtime',client)):
 shutil.copytree(source,dest,dirs_exist_ok=True,ignore=shutil.ignore_patterns('console.log','screenshots','*.mdmp'))
maps=[l.strip() for l in (server/'cstrike/cfg/sourceadvanced_maps.txt').read_text().splitlines() if l.strip() and not l.startswith('#')]
password=secrets.token_hex(16);port=27694
name='connection_'+secrets.token_hex(4)+'.cfg'
scfg=server/'cstrike/cfg'/name;ccfg=client/'cstrike/cfg'/name
stopname='stop_'+secrets.token_hex(4)+'.cfg';stopcfg=client/'cstrike/cfg'/stopname;stopcfg.write_text('',encoding='ascii')
scfg.write_text('net_usesocketsforloopback 1\nsv_lan 1\nsv_cheats 1\nsv_hibernate_when_empty 0\nsv_allow_wait_command 1\nbot_quota 0\n',encoding='ascii')
ccfg.write_text('net_usesocketsforloopback 1\nhost_competitive_ever_enabled 1\nfps_max 60\nengine_no_focus_sleep 0\ncl_profile_map_load 1\nsv_allow_wait_command 1\nunbindall\nsensitivity 0\n'+
 f'alias sa_private_profile_poll "exec {stopname}; wait 60; sa_private_profile_poll"\n',encoding='ascii')
clog=client/'cstrike/console.log';clog.unlink(missing_ok=True)
channel=None;cproc=None;forced=False;records=[];initial=None;error=None
started=time.monotonic()
sproc=subprocess.Popen([str(server/'dedicated_launcher.exe'),'-game','cstrike','-console','-condebug','-ip','127.0.0.1','-port',str(port),'-tickrate','128',
 '+sv_lan','1','+clientport','27697','+rcon_password',password,'+servercfgfile',name,'+lservercfgfile',name,'+map','de_mirage_csgo_new'],cwd=server,creationflags=subprocess.CREATE_NO_WINDOW)
print('Private dedicated profile PID',sproc.pid,flush=True)
def wait_map(mapname,start,offset=0,require_client=True):
 global channel
 ready=None;last=''
 while sproc.poll() is None and (cproc is None or cproc.poll() is None) and time.monotonic()-start<600:
  try:
   channel=Rcon(port,password);channel.sock.settimeout(.2);last=channel.command('status')
   if re.search(r'^map\s*:\s*'+re.escape(mapname)+r'\b',last,re.M):
    if ready is None:ready=round(time.monotonic()-start,2)
    if not require_client:return dict(map=mapname,server_ready_seconds=ready,passed=True)
    if clog.exists():
     with clog.open('rb') as stream:stream.seek(offset);text=stream.read().decode('utf8','replace')
    else:text=''
    marker=re.search(r'\[load-profile\] event=client_ready map='+re.escape(mapname)+r' t=([\d.]+)',text)
    if marker and re.search(r'^#\s+\d+\s+"[^\n]+\bactive\b',last,re.M):
     return dict(map=mapname,server_ready_seconds=ready,client_active_seconds=round(time.monotonic()-start,2),client_ready_marker=True,
       phases=[dict(phase=p,milliseconds=float(ms)) for p,ms in re.findall(r'\[load-profile\] event=end phase=(\w+) map='+re.escape(mapname)+r' ms=([\d.]+)',text)],
       signon=[dict(state=int(state),timestamp=float(t)) for state,t in re.findall(r'\[load-profile\] event=signon state=(\d+).*? t=([\d.]+)',text)],status=last,passed=True)
   channel.sock.close();channel=None
  except (OSError,EOFError,RuntimeError):
   if channel:channel.sock.close();channel=None
  time.sleep(.5)
 return dict(map=mapname,server_ready_seconds=ready,client_active_seconds=None,status=last,passed=False)
try:
 boot=wait_map('de_mirage_csgo_new',started,require_client=False)
 if not boot['passed']:raise RuntimeError('Private dedicated did not become ready')
 channel.sock.close();channel=None
 connect_start=time.monotonic()
 cproc=subprocess.Popen([str(client/'hl2_launcher.exe'),'-game','cstrike','-multirun','-windowed','-w','800','-h','600','-novid','-condebug','-port','27696','+clientport','27695',
  '+host_competitive_ever_enabled','1','+cl_profile_map_load','1','+exec',name,'+connect','127.0.0.1:'+str(port),'+sa_private_profile_poll'],cwd=client)
 print('Separate client profile PID',cproc.pid,flush=True)
 initial=wait_map('de_mirage_csgo_new',connect_start)
 print('Initial client connection ready',initial.get('client_active_seconds'),'s',flush=True)
 if not initial['passed']:raise RuntimeError('Private client failed to finish initial dedicated connection')
 for mapname in maps:
  offset=clog.stat().st_size;start=time.monotonic();channel.command('changelevel '+mapname);channel.sock.close();channel=None
  record=wait_map(mapname,start,offset);records.append(record)
  (logs/'progress.json').write_text(json.dumps(records,indent=2),encoding='utf8')
  print(mapname,'dedicated ready',record['server_ready_seconds'],'client ready',record['client_active_seconds'],'PASS' if record['passed'] else 'FAIL',flush=True)
  if not record['passed']:break
 # The private client executes this file through its own console command loop.
 if cproc and cproc.poll() is None:
  stopcfg.write_text('quit\n',encoding='ascii')
  try:cproc.wait(timeout=20)
  except subprocess.TimeoutExpired:forced=True
 if sproc.poll() is None and channel:
  channel.sock.settimeout(2);channel.command('quit')
  try:sproc.wait(timeout=20)
  except subprocess.TimeoutExpired:forced=True
except Exception as exc:
 error=str(exc);print("Dedicated profile error:",error,flush=True)
finally:
 if channel:channel.sock.close()
 for process in (cproc,sproc):
  if process and process.poll() is None:forced=True;process.terminate();process.wait(timeout=10)
 scfg.unlink(missing_ok=True);ccfg.unlink(missing_ok=True);stopcfg.unlink(missing_ok=True)
report=dict(error=error,initial_connection=initial,maps=records,server_exit=sproc.returncode,client_exit=cproc.returncode if cproc else None,forced_shutdown=forced,
 elapsed=round(time.monotonic()-started,2),conditions='Private dedicated 128 tick and separate real client, OS loopback sockets enabled with net_usesocketsforloopback=1 and distinct ports, no bots, windowed 800x600, warm file/shader cache. Changes measured from changelevel to actual client_ready after loading-screen completion. No Internet latency included.',
 passed=len(records)==10 and all(r['passed'] for r in records) and sproc.returncode==0 and cproc.returncode==0 and not forced)
(logs/'result.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('Dedicated loading profile passed:',report['passed'],flush=True)
raise SystemExit(0 if report['passed'] else 1)
