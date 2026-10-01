import os
"""Capture first weapons after death/team changes WITHOUT bone-audit repair."""
from pathlib import Path
import hashlib,json,re,secrets,subprocess,sys,time
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));root=work/'animation-gloves-update';runtime=root/'candidate-runtime'
repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine');logs=root/'arm-lifecycle-test';logs.mkdir(exist_ok=True)
sys.path.insert(0,str(repo/'tools'));from validate_native_instance import Rcon
name='arm_lifecycle_'+secrets.token_hex(4)+'.cfg';cfg=runtime/'cstrike/cfg'/name;action=cfg.with_name('action_'+name)
cfg.write_text('sv_lan 1\nsv_cheats 1\nbot_quota 0\nmp_autoteambalance 0\nmp_limitteams 0\nmp_freezetime 0\nmp_roundtime 60\nfps_max 60\nengine_no_focus_sleep 0\nsv_allow_wait_command 1\ndeveloper 0\nunbindall\nsensitivity 0\ncl_inventory_loadout ""\ninventory_gloves 4019\n',encoding='ascii')
password=secrets.token_hex(16);port=27692
proc=subprocess.Popen([str(runtime/'hl2_launcher.exe'),'-game','cstrike','-multirun','-windowed','-w','1024','-h','768','-novid','-condebug','-usercon','-ip','127.0.0.1','-port',str(port),'-clientport','27693',
 '+rcon_password',password,'+servercfgfile',name,'+lservercfgfile',name,'+map','de_mirage_csgo_new'],cwd=runtime)
print('Private arm lifecycle PID',proc.pid,flush=True);start=time.monotonic();channel=None;error=None;forced=False;records=[]
def command(text):return channel.command(text)
def client(text):
 marker='arm_done_'+secrets.token_hex(6);done=cfg.with_name(marker+'.cfg')
 # File acknowledgement works with developer=0, where echo is not logged.
 action.write_text(text+'\nhost_writeconfig '+marker+'\n',encoding='ascii');command('exec '+action.name)
 deadline=time.monotonic()+75
 while proc.poll() is None and time.monotonic()<deadline:
  if done.is_file() and done.stat().st_size>0:done.unlink();return
  time.sleep(.2)
 raise RuntimeError('Client action did not complete: '+marker)
def capture(stage,setup):
 client(setup+'; hidepanel info; hidepanel team; hidepanel class_ter; hidepanel class_ct; hideconsole; gameui_hide; firstperson; setang 0 0 0; wait 180; cl_screenshotname arms_'+stage+'; screenshot; wait 30')
 state=command('cs_economy_status');render=command('cl_unified_arms_status')
 records.append(dict(stage=stage,state=state,render=render,capture=str(runtime/'cstrike/screenshots'/('arms_'+stage+'.tga'))));print(stage,state.strip(),render.strip(),flush=True)
def join(team,cls):
 client(f'cmd jointeam {team}; wait 120; cmd joinclass {cls}; wait 180')
 command('mp_restartgame 1');time.sleep(4)
 deadline=time.monotonic()+25
 while time.monotonic()<deadline:
  if re.search(r'index=1 team='+str(team)+r' alive=1',command('cs_economy_status')):return
  time.sleep(.5)
 raise RuntimeError('Team/respawn transition did not complete')
try:
 deadline=start+420
 while proc.poll() is None and time.monotonic()<deadline:
  try:
   channel=Rcon(port,password);status=command('status')
   if re.search(r'^#\s+\d+\s+"[^\n]+\bactive\b',status,re.M):break
   channel.sock.close();channel=None
  except (OSError,EOFError,RuntimeError):
   if channel:channel.sock.close();channel=None
  time.sleep(1)
 if not channel:raise RuntimeError('Private client did not become active')
 join(3,5);capture('ct_first','use weapon_usp')
 client('cmd kill; wait 120');command('mp_restartgame 1');time.sleep(4)
 capture('ct_respawn','use weapon_usp')
 join(2,1);capture('t_first','use weapon_glock')
 join(3,5);capture('ct_team_return','use weapon_usp')
 capture('ct_awp','give weapon_awp; use weapon_awp; wait 180')
 capture('ct_usp_again','use weapon_usp; wait 180')
 client('cmd kill; wait 120');command('mp_restartgame 1');time.sleep(4)
 capture('deagle_first','ent_remove_all weapon_usp; give weapon_deagle; use weapon_deagle; wait 180')
 records.append(dict(final_audit=command('cl_inventory_validate')))
 command('quit');proc.wait(timeout=30)
except Exception as exc:error=str(exc);print('Lifecycle test error:',error,flush=True)
finally:
 if channel:channel.sock.close()
 if proc.poll() is None:forced=True;proc.terminate();proc.wait(timeout=10)
 cfg.unlink(missing_ok=True);action.unlink(missing_ok=True)
report=dict(error=error,exit=proc.returncode,forced_shutdown=forced,elapsed=round(time.monotonic()-start,2),records=records,
 modules={r:hashlib.sha256((runtime/r).read_bytes()).hexdigest() for r in ('cstrike/bin/client.dll','cstrike/bin/server.dll','bin/GameUI.dll')},
 completed=bool(not error and proc.returncode==0 and not forced and len(records)==8),
 attachment_valid=all("attached=1" in r["render"] and "should_draw=1" in r["render"] and "ret=1" in r["render"] and "alive=1" in r["state"] for r in records if "stage" in r),visual_review_pending=True)
(logs/'result.json').write_text(json.dumps(report,indent=2));print('Lifecycle captures complete:',report['completed'],flush=True)
raise SystemExit(0 if report['completed'] else 1)
