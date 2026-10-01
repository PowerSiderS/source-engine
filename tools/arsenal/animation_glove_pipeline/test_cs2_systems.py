import os
"""Private native purchases, official-data checks, posed hitboxes and HUD captures."""
from pathlib import Path
import csv,hashlib,json,math,re,secrets,subprocess,sys,time
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
root=work/'animation-gloves-update';runtime=root/'candidate-runtime';logs=root/'cs2-systems-test';logs.mkdir(exist_ok=True)
sys.path.insert(0,str(repo/'tools'));from validate_native_instance import Rcon
name='cs2_systems_'+secrets.token_hex(4)+'.cfg';action='cs2_actions_'+secrets.token_hex(4)+'.cfg'
cfg=runtime/'cstrike/cfg'/name;acfg=cfg.with_name(action)
cfg.write_text('host_competitive_ever_enabled 1\nsv_lan 1\nsv_cheats 1\nmp_autoteambalance 0\nmp_limitteams 0\nbot_quota 0\nmp_freezetime 0\nmp_roundtime 60\nmp_buytime 60\nmp_halftime 0\nmp_match_can_clinch 0\nmp_maxrounds 100\nfps_max 60\nengine_no_focus_sleep 0\nsv_allow_wait_command 1\ndeveloper 0\ncl_profile_map_load 1\ncl_disablehtmlmotd 1\nunbindall\nsensitivity 0\ncl_inventory_loadout ""\ninventory_gloves 4019\n',encoding='ascii')
password=secrets.token_hex(16);port=27688;log=runtime/'cstrike/console.log';log.unlink(missing_ok=True)
proc=subprocess.Popen([str(runtime/'hl2_launcher.exe'),'-game','cstrike','-multirun','-windowed','-w','1024','-h','768','-novid','-condebug','-usercon','-ip','127.0.0.1','-port',str(port),'-clientport','27689','-tickrate','128',
 '+rcon_password',password,'+servercfgfile',name,'+lservercfgfile',name,'+map','de_mirage_csgo_new'],cwd=runtime)
print('Private CS2 systems PID',proc.pid,flush=True)
start=time.monotonic();channel=None;responses={};checks=[];poses=[];error=None;forced=False
def command(text):
 answer=channel.command(text);responses[text]=answer;return answer
def client(text):
 marker='cs2_done_'+secrets.token_hex(6);done=cfg.with_name(marker+'.cfg')
 acfg.write_text(text+'\nhost_writeconfig '+marker+'\n',encoding='ascii');command('exec '+action)
 deadline=time.monotonic()+90
 while proc.poll() is None and time.monotonic()<deadline:
  if done.is_file() and done.stat().st_size>0:done.unlink();return
  time.sleep(.2)
 raise RuntimeError('Client action did not complete: '+marker)
def check(title,actual,expected):
 passed=actual==expected;checks.append(dict(test=title,actual=actual,expected=expected,passed=passed));print(title,actual,'PASS' if passed else 'FAIL',flush=True)
def balance():
 text=command('cs_economy_status');m=re.search(r'index=1 team=(\d+) alive=(\d+) cash=(\d+)',text)
 if not m:raise RuntimeError('Local player balance unavailable: '+text[:200])
 return dict(team=int(m[1]),alive=int(m[2]),cash=int(m[3]))
def buycase(label,buy,cost,setup=''):
 if setup:client(setup)
 client('impulse 101; wait 60');before=balance()['cash'];client('cmd buy '+buy+'; wait 120');after=balance()['cash'];check(label,before-after,cost)
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
 if not channel:raise RuntimeError('Client never became active')
 client('cmd jointeam 2; wait 120; cmd joinclass 1; wait 180; hidepanel info; hidepanel team; hidepanel class_ter; hidepanel class_ct; gameui_hide; firstperson')
 time.sleep(3)
 for cmd in ('cs_validate_economy','cs_validate_weapon_balance','cs_export_gunplay','cs_export_player_hitboxes'):
  command(cmd)
 prices=json.loads((work/'cs2-economy-reference/weapon_economy.json').read_text())
 economy=responses['cs_validate_economy'];entries=re.findall(r'\[economy-audit\] PASS (\S+) reference=(\S+) price=(\d+) kill=(\d+)',economy)
 economic_errors=[]
 for weapon,reference,price,kill in entries:
  original=prices.get('weapon_'+reference)
  if original and (int(price)!=original.get('m_nPrice',int(price)) or int(kill)!=original.get('m_nKillAward',300)):economic_errors.append(weapon)
 check('38 effective weapon prices/rewards',len(entries),38);check('Prices equal independent CS2 extraction',economic_errors,[])
 profiles=json.loads((repo/'references/cs2/weapon_profiles.json').read_text())['profiles'];expected={p['id'].removeprefix('WEAPON_').lower():p['values'] for p in profiles}
 fields={'seed':'m_iRecoilSeed','cycle':'m_flCycleTime[{m}]','spread':'m_fSpread[{m}]','crouch':'m_fInaccuracyCrouch[{m}]','stand':'m_fInaccuracyStand[{m}]','move':'m_fInaccuracyMove[{m}]','jump':'m_fInaccuracyJump[{m}]','fire':'m_fInaccuracyImpulseFire[{m}]',
 'recovery_crouch':'m_fRecoveryTimeCrouch','recovery_crouch_final':'m_fRecoveryTimeCrouchFinal','recovery_stand':'m_fRecoveryTimeStand','recovery_stand_final':'m_fRecoveryTimeStandFinal','land':'m_fInaccuracyLand[{m}]','ladder':'m_fInaccuracyLadder[{m}]','jump_initial':'m_fInaccuracyJumpInitial[{m}]',
 'recoil_angle':'m_fRecoilAngle[{m}]','recoil_angle_variance':'m_fRecoilAngleVariance[{m}]','recoil_magnitude':'m_fRecoilMagnitude[{m}]','recoil_magnitude_variance':'m_fRecoilMagnitudeVariance[{m}]','transition_start':'m_iRecoveryTransitionStartBullet','transition_end':'m_iRecoveryTransitionEndBullet','damage':'m_iDamage','armor_ratio':'m_flArmorRatio','headshot_multiplier':'m_flHeadshotMultiplier','range':'m_flRange','range_modifier':'m_flRangeModifier','clip':'iMaxClip1'}
 rows=list(csv.DictReader((runtime/'cstrike/gunplay_audit.csv').open()));differences=[]
 for row in rows:
  alias=row['weapon'].removeprefix('weapon_');p=expected[alias];mode=int(row['mode'])
  for key,member in fields.items():
   if not math.isclose(float(row[key]),float(p[member.format(m=mode)]),rel_tol=2e-6,abs_tol=2e-8):differences.append([alias,mode,key,row[key],p[member.format(m=mode)]])
  for key in ('angle','magnitude'):
   if not math.isfinite(float(row[key])):differences.append([alias,mode,'nonfinite'])
 check('34 weapons x 2 modes x 64 recoil impulses',len(rows),4352);check('Loaded gunplay equals extracted CS2 values',differences,[])
 export=runtime/'cstrike/sourceadvanced_gameplay/models/player';assetroot=work/'cs2-hitbox-import/assets/cstrike/models/player'
 mounted=[]
 for model in assetroot.glob('*.mdl'):
  if not (export/model.name).exists() or model.read_bytes()!=(export/model.name).read_bytes():mounted.append(model.name)
 check('Mounted MDLs equal eight adapted CS2 hitbox models',mounted,[])
 # Actual fire exercises every compiled profile, including the ten weapons
 # not represented by the supplied 24-model animation pack.
 aliases=['weapon_'+p['id'].removeprefix('WEAPON_').lower() for p in profiles]
 clear='\n'.join('ent_remove_all '+alias for alias in aliases)
 client('setang 0 0 0')
 for alias in aliases:
  client(clear+'\ninventory_unequip\ngive '+alias+'\nuse '+alias+'\nwait 180')
  before=command('cl_inventory_validate');a=re.search(r'clip=(\d+)/(\d+)',before)
  client('+attack; wait 60; -attack; wait 30')
  after=command('cl_inventory_validate');b=re.search(r'clip=(\d+)/(\d+)',after)
  check('Actual discharge '+alias,bool(a and b and int(a[1])>int(b[1]) and a[2]==b[2]),True)
 client(clear+'\n-attack')
 buycase('AK-47 actual debit','ak47',2700,'ent_remove_all weapon_ak47')
 buycase('Already-owned AK-47 charges nothing','ak47',0)
 buycase('CT-only M4 denied on T','m4a1',0)
 buycase('AWP actual debit','awp',4750,'ent_remove_all weapon_awp; ent_remove_all weapon_ak47')
 client('use weapon_awp; wait 120');command('cl_inventory_validate')
 check('AWP mechanically has five rounds','clip=5/5' in responses['cl_inventory_validate'],True)
 buycase('Desert Eagle actual debit','deagle',700,'ent_remove_all weapon_deagle')
 buycase('HE actual debit','hegrenade',300,'ent_remove_all weapon_hegrenade')
 buycase('Smoke actual debit','smokegrenade',300,'ent_remove_all weapon_smokegrenade')
 buycase('Flash actual debit','flashbang',200,'ent_remove_all weapon_flashbang')
 client('impulse 101; wait 60; cmd buyammo1; cmd buyammo2; wait 120');check('Ammo replenishment has no cash charge',balance()['cash'],16000)
 for team in (2,3):
  if balance()['team']!=team:client(f'cmd jointeam {team}; wait 180; cmd joinclass 1; wait 180')
  for cls in (range(1,5) if team==2 else range(5,9)):
   client(f'cmd joinclass {cls}; wait 180');command('mp_restartgame 1');time.sleep(4)
   record=dict(team=team,player_class=cls)
   for pose,inputs in [('standing','-duck; -forward; setang 0 0 0'),('crouched','+duck'),('turned','-duck; setang 0 90 0')]:
    client(inputs+'; wait 180');result=command('cs_validate_hitboxes');record[pose]=result
    summary=re.search(r'players=(\d+) rays=(\d+) failures=(\d+)',result)
    check(f'Hitbox team={team} class={cls} {pose}',bool(summary and int(summary[1])>0 and int(summary[2])>0 and summary[3]=='0'),True)
   poses.append(record)
 models={m for pose in poses for result in (pose.get('standing',''),) for m in re.findall(r'model=(\S+) bones=',result)}
 check('All eight player rigs exercised in animated engine',len(models),8)
 client('-duck; setang -25 0 0; give weapon_ak47; use weapon_ak47; wait 120; inventory_gloves 4019; wait 120; developer 0; hideconsole; gameui_hide')
 for w,h in ((800,600),(1024,768),(1920,1080)):
  client(f'mat_setvideomode {w} {h} 1; wait 180; cl_screenshotname hud_csgo_{w}; screenshot; wait 30')
  print('HUD capture',w,h,flush=True)
 client('open_inventory; wait 180; cl_screenshotname inventory_csgo; screenshot; wait 30')
 command('quit');proc.wait(timeout=30)
except Exception as exc:
 error=str(exc);print('Systems test error:',error,flush=True)
finally:
 if channel:channel.sock.close()
 if proc.poll() is None:forced=True;proc.terminate();proc.wait(timeout=10)
 cfg.unlink(missing_ok=True);acfg.unlink(missing_ok=True)
report=dict(error=error,exit=proc.returncode,forced_shutdown=forced,elapsed=round(time.monotonic()-start,2),checks=checks,poses=poses,responses=responses,
 modules={r:hashlib.sha256((runtime/r).read_bytes()).hexdigest() for r in ('cstrike/bin/client.dll','cstrike/bin/server.dll','bin/GameUI.dll')})
report['passed']=bool(not error and proc.returncode==0 and not forced and checks and all(c['passed'] for c in checks))
(logs/'result.json').write_text(json.dumps(report,indent=2));print('CS2 systems passed:',report['passed'],flush=True)
raise SystemExit(0 if report['passed'] else 1)
