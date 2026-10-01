import os
"""Generate shared Source 1 gunplay profiles from the installed CS2 KV3 snapshot.

Values retain their authored units (recoil angles in degrees, time in seconds).
Source 2's closed-source
subtick simulation is not exported or claimed to be reproduced here.
"""
from pathlib import Path
import hashlib,json,re
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'))
repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
decoded=work/'cs2-economy-reference/decoded'
text=decoded.read_text()
economy=(repo/'game/shared/cstrike/cs_economy.h').read_text()
pairs=re.findall(r'\{ (WEAPON_\w+), "([^"]+)", (\d+), (\d+) \}',economy)
scalars={'m_iRecoilSeed':'m_nRecoilSeed','m_bFullAuto':'m_bIsFullAuto',
 'iMaxClip1':'m_iMaxClip1','m_iDamage':'m_nDamage','m_iBullets':'m_nNumBullets',
 'm_flArmorRatio':'m_flArmorRatio','m_flHeadshotMultiplier':'m_flHeadshotMultiplier','m_flRange':'m_flRange','m_flRangeModifier':'m_flRangeModifier',
 'm_fRecoveryTimeCrouch':'m_flRecoveryTimeCrouch','m_fRecoveryTimeCrouchFinal':'m_flRecoveryTimeCrouchFinal',
 'm_fRecoveryTimeStand':'m_flRecoveryTimeStand','m_fRecoveryTimeStandFinal':'m_flRecoveryTimeStandFinal',
 'm_iRecoveryTransitionStartBullet':'m_nRecoveryTransitionStartBullet',
 'm_iRecoveryTransitionEndBullet':'m_nRecoveryTransitionEndBullet','m_fInaccuracyReload':'m_flInaccuracyReload'}
arrays={'m_fSpread':'m_flSpread','m_fInaccuracyCrouch':'m_flInaccuracyCrouch','m_fInaccuracyStand':'m_flInaccuracyStand',
 'm_fInaccuracyJump':'m_flInaccuracyJump','m_fInaccuracyLand':'m_flInaccuracyLand','m_fInaccuracyLadder':'m_flInaccuracyLadder',
 'm_fInaccuracyImpulseFire':'m_flInaccuracyFire','m_fInaccuracyMove':'m_flInaccuracyMove',
 'm_fRecoilAngle':'m_flRecoilAngle','m_fRecoilAngleVariance':'m_flRecoilAngleVariance',
 'm_fRecoilMagnitude':'m_flRecoilMagnitude','m_fRecoilMagnitudeVariance':'m_flRecoilMagnitudeVariance'}
profiles=[]
for enum,name,_,_ in pairs:
 match=re.search(r'\bweapon_'+re.escape(name)+r'_prefab\s*=\s*\{',text)
 if not match:continue
 start=match.end();depth=1;end=start
 while depth:
  if text[end]=='{':depth+=1
  elif text[end]=='}':depth-=1
  end+=1
 block=text[start:end-1]
 def value(key):
  raw=re.search(r'^\s*'+key+r'\s*=\s*([^\r\n]+)',block,re.M)
  if not raw:raise ValueError(name+': missing '+key)
  return json.loads(raw[1].strip())
 if value('m_iMaxClip1')<=0:continue
 record={'id':enum,'reference':'weapon_'+name,'values':{}}
 for member,key in scalars.items():record['values'][member]=value(key)
 record['values']['iDefaultClip1']=record['values']['iMaxClip1']
 speed=value('m_flMaxSpeed');record['values'].update(m_flMaxSpeed=speed[0],m_flMaxSpeedAlt=speed[1])
 for member,key in arrays.items():
  data=value(key)
  if isinstance(data,(int,float)):data=[data,data]
  assert len(data)==2,(name,key,data)
  for mode in (0,1):record['values'][member+f'[{mode}]']=data[mode]
 for mode in (0,1):
  cycle=value('m_flCycleTime');initial=value('m_flInaccuracyJumpInitial')
  record['values'][f'm_flCycleTime[{mode}]']=cycle[mode] if isinstance(cycle,list) else cycle
  record['values'][f'm_fInaccuracyJumpInitial[{mode}]']=initial[mode] if isinstance(initial,list) else initial
 profiles.append(record)
assert len(profiles)==34,len(profiles)
def literal(value):
 if isinstance(value,bool):return str(value).lower()
 if isinstance(value,int):return str(value)
 return repr(float(value))+'f'
lines=['// Generated from installed Valve CS2 weapons.vdata_c; snapshot 2026-10-01.',
 '// Source 1 class aliases are explicit in references/cs2/weapon_profiles.json.',
 '// Subtick timing, spread RNG and Source 2-only mechanics are separate systems.',
 '#ifndef CS2_WEAPON_PROFILES_H','#define CS2_WEAPON_PROFILES_H','#include "cs_weapon_parse.h"',
 'inline bool CSApplyCS2WeaponProfile(CCSWeaponInfo &info, CSWeaponID id)', '{','    switch(id)','    {']
for record in profiles:
 lines.append('    case '+record['id']+': // '+record['reference'])
 for member,value in record['values'].items():lines.append('        info.'+member+' = '+literal(value)+';')
 alias=record['id'].removeprefix('WEAPON_').lower()
 lines.append('        Q_strncpy(info.aShootSounds[SINGLE], "SACS2.Fire.'+alias+'", sizeof(info.aShootSounds[SINGLE]));')
 if alias in ('usp','m4a1'):
  # SPECIAL1 is the silenced mode in this Source 1 fork.
  lines.append('        Q_strncpy(info.aShootSounds[SPECIAL1], "SACS2.Fire.'+alias+'", sizeof(info.aShootSounds[SPECIAL1]));')
  lines.append('        Q_strncpy(info.aShootSounds[SINGLE], "SACS2.Fire.'+alias+'_alt", sizeof(info.aShootSounds[SINGLE]));')
 lines.append('        return true;')
lines+=['    default: return false;','    }','}','#endif','']
(repo/'game/shared/cstrike/cs2_weapon_profiles.h').write_text('\n'.join(lines))
reference=repo/'references/cs2';reference.mkdir(parents=True,exist_ok=True)
report={'snapshot':'2026-10-01','source':'installed Valve CS2 scripts/weapons.vdata_c',
 'compiled_resource_sha256':hashlib.sha256((decoded.parent/'weapons.vdata_c').read_bytes()).hexdigest(),
 'decoded_sha256':hashlib.sha256(decoded.read_bytes()).hexdigest(),'count':len(profiles),'profiles':profiles,
 'limitations':['Source 1 deterministic recoil generator is retained; no equivalence to Source 2 subtick/RNG claimed.',
 'P228/Scout/M3/TMP/SG550/SG552/MP5 map to P250/SSG08/Nova/MP9/SCAR20/SG556/MP5SD.',
 'Source 2 penetration power/jump-apex require dedicated simulation work.']}
(reference/'weapon_profiles.json').write_text(json.dumps(report,indent=2))
print('Generated',len(profiles),'compiled CS2 gunplay profiles with source hashes.')
