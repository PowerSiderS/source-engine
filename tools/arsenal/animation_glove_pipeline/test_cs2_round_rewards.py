import os
"""Actual round transitions, with frozen private bots and independent expectations."""
from pathlib import Path
import hashlib,json,re,secrets,subprocess,sys,time
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
root=work/'animation-gloves-update';runtime=root/'candidate-dedicated-runtime';logs=root/'round-economy-test';logs.mkdir(exist_ok=True)
sys.path.insert(0,str(repo/'tools'));from validate_native_instance import Rcon
port=27690;password=secrets.token_hex(16);name='round_economy_'+secrets.token_hex(4)+'.cfg';cfg=runtime/'cstrike/cfg'/name
cfg.write_text('sv_lan 1\nsv_cheats 1\nsv_hibernate_when_empty 0\nbot_stop 1\nbot_quota_mode normal\nbot_quota 4\nbot_join_after_player 0\nbot_join_team any\nmp_autoteambalance 0\nmp_limitteams 0\nmp_freezetime 0\nmp_round_restart_delay 1\nmp_halftime 0\nmp_match_can_clinch 0\nmp_maxrounds 100\n',encoding='ascii')
proc=subprocess.Popen([str(runtime/'dedicated_launcher.exe'),'-game','cstrike','-console','-condebug','-usercon','-ip','127.0.0.1','-port',str(port),'-tickrate','128',
 '+rcon_password',password,'+servercfgfile',name,'+lservercfgfile',name,'+map','de_mirage_csgo_new'],cwd=runtime,creationflags=subprocess.CREATE_NO_WINDOW)
print('Private round economy PID',proc.pid,flush=True);start=time.monotonic();records=[];error=None;forced=False
def command(text,timeout=75):
 deadline=time.monotonic()+timeout
 while proc.poll() is None and time.monotonic()<deadline:
  channel=None
  try:
   channel=Rcon(port,password);result=channel.command(text)
   if result or text!='cs_economy_status':return result
  except (OSError,EOFError,RuntimeError):pass
  finally:
   if channel:channel.sock.close()
  time.sleep(1)
 raise RuntimeError('Private dedicated stopped responding')
def status():
 text=command('cs_economy_status');header=re.search(r'ct_level=(\d+) t_level=(\d+).*?rounds=(\d+) ct_wins=(\d+) t_wins=(\d+)',text)
 if not header:raise RuntimeError('Missing economic round state')
 players=[dict(index=int(i),team=int(t),alive=int(a),cash=int(c)) for i,t,a,c in re.findall(r'index=(\d+) team=(\d+) alive=(\d+) cash=(\d+)',text)]
 return dict(ct_level=int(header[1]),t_level=int(header[2]),rounds=int(header[3]),ct_wins=int(header[4]),t_wins=int(header[5]),players=players,text=text)
try:
 deadline=start+420
 while time.monotonic()<deadline:
  try:
   current=status()
   if len(current['players'])==4 and all(p['alive'] for p in current['players']):break
  except RuntimeError:
   if proc.poll() is not None:raise
  time.sleep(1)
 command('mp_restartgame 1');time.sleep(4);current=status()
 assert current['ct_level']==current['t_level']==1,current
 for killed_team,ct_level,t_level,ct_award,t_award in [('t',0,2,3250,1900),('t',0,3,3250,2400),('ct',1,2,1400,3250),('t',0,3,3250,2400)]:
  before=current;command('bot_kill all '+killed_team)
  deadline=time.monotonic()+35
  while time.monotonic()<deadline:
   time.sleep(1);current=status()
   if current['rounds']>before['rounds'] and all(p['alive'] for p in current['players']):break
  old={p['index']:p for p in before['players']};changes=[]
  for player in current['players']:
   previous=old[player['index']];award=ct_award if player['team']==3 else t_award
   expected=min(16000,previous['cash']+award);changes.append(dict(index=player['index'],team=player['team'],cash=player['cash'],expected=expected,passed=player['cash']==expected))
  passed=current['ct_level']==ct_level and current['t_level']==t_level and all(c['passed'] for c in changes)
  record=dict(killed_team=killed_team,before=before,after=current,expected_levels=[ct_level,t_level],cash=changes,passed=passed)
  records.append(record);print('Round',current['rounds'],'levels',current['ct_level'],current['t_level'],'PASS' if passed else 'FAIL',flush=True)
 command('quit');proc.wait(timeout=30)
except Exception as exc:error=str(exc);print('Round test error:',error,flush=True)
finally:
 if proc.poll() is None:forced=True;proc.terminate();proc.wait(timeout=10)
 cfg.unlink(missing_ok=True)
report=dict(error=error,exit=proc.returncode,forced_shutdown=forced,elapsed=round(time.monotonic()-start,2),records=records,
 server_sha256=hashlib.sha256((runtime/'cstrike/bin/server.dll').read_bytes()).hexdigest(),
 passed=bool(not error and proc.returncode==0 and not forced and len(records)==4 and all(r['passed'] for r in records)))
(logs/'result.json').write_text(json.dumps(report,indent=2));print('Round rewards passed:',report['passed'],flush=True)
raise SystemExit(0 if report['passed'] else 1)
