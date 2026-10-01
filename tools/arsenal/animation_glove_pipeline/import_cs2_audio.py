import os
"""CRC-checked local CS2 shot audio -> mono PCM16 Source 1 assets."""
from pathlib import Path
import hashlib,io,json,subprocess,wave
import numpy as np
from read_vpk import VPK
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
root=work/'cs2-audio-import';assets=root/'assets/cstrike';raw=root/'raw';raw.mkdir(parents=True,exist_ok=True)
vpk=VPK(r'C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\csgo\pak01_dir.vpk')
cli=work/'cs2-economy-reference/vrf-cli/Source2Viewer-CLI.exe'
profiles=json.loads((repo/'references/cs2/weapon_profiles.json').read_text())['profiles']
custom={'glock':'glock18/glock_01','elite':'elite/elites_01','galilar':'galilar/galil_01','cz75a':'cz75a/cz75_01',
 'mp5sd':'mp5/mp5_01','m4a1_silencer':'m4a1/m4a1_01','m4a1':'m4a1/m4a1_us_01',
 'usp_silencer':'usp/usp_01','nova':'nova/nova-1','sawedoff':'sawedoff/sawedoff-1',
 'xm1014':'xm1014/xm1014-1','revolver':'revolver/revolver-1_01','tec9':'tec9/tec9_02','ump45':'ump45/ump45_02'}
records=[];definitions=[]
requests=[(p['id'].removeprefix('WEAPON_').lower(),p['reference'].removeprefix('weapon_')) for p in profiles]
requests += [('usp_alt','usp_unsilenced'),('m4a1_alt','m4a1_unsilenced')]
custom.update(usp_unsilenced='usp/usp_unsilenced_01',m4a1_unsilenced='m4a1/m4a1_us_01')
for alias,reference in requests:
 name='sounds/weapons/'+custom.get(reference,reference+'/'+reference+'_01')+'.vsnd_c'
 assert name in vpk.entries,name
 source=raw/(alias+'.vsnd_c');data=vpk.get(name);source.write_bytes(data)
 decoded=raw/(alias+'.wav')
 if not decoded.exists():
  subprocess.run([str(cli),'-i',str(source),'-o',str(decoded),'-d'],check=True,stdout=subprocess.DEVNULL,creationflags=subprocess.CREATE_NO_WINDOW)
 with wave.open(str(decoded),'rb') as sound:
  channels,width,rate=sound.getnchannels(),sound.getsampwidth(),sound.getframerate();frames=sound.readframes(sound.getnframes())
 assert width==2,(name,width)
 samples=np.frombuffer(frames,dtype='<i2').reshape(-1,channels)
 mono=np.rint(samples.astype(np.float64).mean(axis=1)).clip(-32768,32767).astype('<i2')
 dest=assets/'sound/sourceadvanced/cs2'/(alias+'.wav');dest.parent.mkdir(parents=True,exist_ok=True)
 with wave.open(str(dest),'wb') as sound:
  sound.setnchannels(1);sound.setsampwidth(2);sound.setframerate(rate);sound.writeframes(mono.tobytes())
 assert len(mono)>0 and int(np.max(np.abs(mono.astype(np.int32))))>0
 event='SACS2.Fire.'+alias
 definitions.append(f'"{event}"\n{{\n "channel" "CHAN_WEAPON"\n "volume" "1.0"\n "pitch" "100"\n "soundlevel" "SNDLVL_GUNFIRE"\n "wave" "sourceadvanced/cs2/{alias}.wav"\n}}\n')
 records.append(dict(event=event,source=name,source_sha256=hashlib.sha256(data).hexdigest(),
  output_sha256=hashlib.sha256(dest.read_bytes()).hexdigest(),channels=1,bits=16,rate=rate,seconds=round(len(mono)/rate,3)))
script=assets/'scripts/game_sounds_sourceadvanced_cs2.txt';script.parent.mkdir(parents=True,exist_ok=True)
script.write_text((work/'cs2-animation-import/assets/cstrike/scripts/game_sounds_sourceadvanced_cs2.txt').read_text()+'\n'+'\n'.join(definitions))
(root/'asset-audit.json').write_text(json.dumps(dict(passed=len(records)==36,records=records,limitations='One-shot mono conversion; Source 2 multi-layer/distance/HRTF mixing is not reproduced.'),indent=2))
print('Converted',len(records),'CRC-checked CS2 shot sounds to mono PCM16.')
