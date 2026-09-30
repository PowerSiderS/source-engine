"""Compile archived Valve weapon data, retaining an audit trail for each value."""
from pathlib import Path
import re, json, hashlib

PROJECT = Path(__file__).resolve().parent.parent
ROOT = PROJECT / 'references/csgo2015'
SOURCE = PROJECT

def parse_kv(text):
    text = re.sub(r'//[^\r\n]*', '', text)
    tokens = re.findall(r'"((?:\\.|[^"\\])*)"|([{}])|([^\s{}"]+)', text)
    tokens = [a if a else b if b else c for a,b,c in tokens]
    index = 0
    def block():
        nonlocal index
        result = {}
        while index < len(tokens):
            key = tokens[index]; index += 1
            if key == '}': return result
            value = tokens[index]; index += 1
            result[key] = block() if value == '{' else value
        return result
    return block()

def merge(target, incoming):
    for key,value in incoming.items():
        if isinstance(value,dict): merge(target.setdefault(key,{}),value)
        else: target[key] = value

game = parse_kv((ROOT/'items_game-2015-11-11.txt').read_text())['items_game']
prefabs = game['prefabs']
def resolve(entry, seen=()):
    out = {}
    for name in entry.get('prefab','').split():
        assert name not in seen, name
        merge(out, resolve(prefabs[name],seen+(name,)))
    merge(out, entry)
    return out

# Explicit gameplay mappings for the existing CS:S classes. These mappings do
# not claim that the old models/animations are the corresponding CS:GO models.
MAPPING = {
 'WEAPON_P228':'weapon_p250', 'WEAPON_GLOCK':'weapon_glock',
 'WEAPON_SCOUT':'weapon_ssg08', 'WEAPON_XM1014':'weapon_xm1014',
 'WEAPON_MAC10':'weapon_mac10', 'WEAPON_AUG':'weapon_aug',
 'WEAPON_ELITE':'weapon_elite','WEAPON_FIVESEVEN':'weapon_fiveseven',
 'WEAPON_UMP45':'weapon_ump45','WEAPON_SG550':'weapon_scar20',
 'WEAPON_GALIL':'weapon_galilar','WEAPON_FAMAS':'weapon_famas',
 'WEAPON_USP':'weapon_usp_silencer','WEAPON_AWP':'weapon_awp',
 'WEAPON_MP5NAVY':'weapon_mp7','WEAPON_M249':'weapon_m249',
 'WEAPON_M3':'weapon_nova','WEAPON_M4A1':'weapon_m4a1_silencer',
 'WEAPON_TMP':'weapon_mp9','WEAPON_G3SG1':'weapon_g3sg1',
 'WEAPON_DEAGLE':'weapon_deagle','WEAPON_SG552':'weapon_sg556',
 'WEAPON_AK47':'weapon_ak47','WEAPON_P90':'weapon_p90',
 'WEAPON_CZ75':'weapon_cz75a','WEAPON_TEC9':'weapon_tec9','WEAPON_P2000':'weapon_hkp2000',
 'WEAPON_M4A4':'weapon_m4a1','WEAPON_MP7':'weapon_mp7','WEAPON_BIZON':'weapon_bizon',
 'WEAPON_MAG7':'weapon_mag7','WEAPON_SAWEDOFF':'weapon_sawedoff','WEAPON_NEGEV':'weapon_negev',
}

fields = [('cycle','CycleTime','cycletime',1), ('spread','Spread','spread',.001),
 ('crouch','InaccuracyCrouch','inaccuracy crouch',.001),
 ('stand','InaccuracyStand','inaccuracy stand',.001),
 ('jump','InaccuracyJump','inaccuracy jump',.001),
 ('land','InaccuracyLand','inaccuracy land',.001),
 ('ladder','InaccuracyLadder','inaccuracy ladder',.001),
 ('fire','InaccuracyFire','inaccuracy fire',.001),
 ('move','InaccuracyMove','inaccuracy move',.001),
 ('angle','RecoilAngle','recoil angle',1),
 ('angleVariance','RecoilAngleVariance','recoil angle variance',1),
 ('magnitude','RecoilMagnitude','recoil magnitude',1),
 ('magnitudeVariance','RecoilMagnitudeVariance','recoil magnitude variance',1)]
profiles=[]
for ident,name in MAPPING.items():
    entry=next(resolve(v) for v in game['items'].values() if v.get('name')==name)
    script_name=entry.get('item_class',name)
    script=ROOT/'official-scripts'/(script_name+'.txt')
    raw={k.lower():v for k,v in parse_kv(script.read_text())['WeaponData'].items()}
    attrs=entry.get('attributes',{})
    def number(script_key,attr_key,default):
        value=attrs.get(attr_key,raw.get(script_key.lower(),default))
        if isinstance(value,dict): value=value['value']
        return float(value)
    p={'id':ident,'name':name,'base_script':script.name,'script_sha256':hashlib.sha256(script.read_bytes()).hexdigest(),
       'seed':int(number('RecoilSeed','recoil seed',0)),
       'fullAuto':bool(number('FullAuto','is full auto',0)),
       'maxSpeed':number('MaxPlayerSpeed','maximum player speed',250),
       'maxSpeedAlt':number('MaxPlayerSpeedAlt','maximum player speed alt',number('MaxPlayerSpeed','maximum player speed',250)),
       'recoveryCrouch':number('RecoveryTimeCrouch','recovery time crouch',1),
       'recoveryStand':number('RecoveryTimeStand','recovery time stand',1),'modes':[]}
    for mode in range(2):
        p['modes'].append({field:number(sk+('Alt' if mode else ''),ak+(' alt' if mode else ''),
            number(sk,ak,0))*scale for field,sk,ak,scale in fields})
    profiles.append(p)

(ROOT/'historical-profiles.json').write_text(json.dumps(profiles,indent=2))
header='''// Generated from Valve scripts archived 2015-11-30 and Valve items_game dated 2015-11-11.
// See references/csgo2015/provenance.json. No modern recovery-final/jump-apex values.
#ifndef CS_2015_WEAPON_PROFILES_H
#define CS_2015_WEAPON_PROFILES_H
#include "cs_weapon_parse.h"
struct CS2015WeaponMode { float cycle, spread, crouch, stand, jump, land, ladder, fire, move, angle, angleVariance, magnitude, magnitudeVariance; };
struct CS2015WeaponProfile { CSWeaponID id; const char *name; int seed; bool fullAuto; float maxSpeed, maxSpeedAlt, recoveryCrouch, recoveryStand; CS2015WeaponMode modes[2]; };
static const CS2015WeaponProfile g_CS2015WeaponProfiles[] = {
'''
fmt=lambda x:format(x,'.9g')+('f' if '.' in format(x,'.9g') or 'e' in format(x,'.9g') else '.0f')
for p in profiles:
    modes=', '.join('{ '+', '.join(fmt(m[k]) for k,sk,ak,s in fields)+' }' for m in p['modes'])
    header+=f'    {{ {p["id"]}, "{p["name"]}", {p["seed"]}, {str(p["fullAuto"]).lower()}, {fmt(p["maxSpeed"])}, {fmt(p["maxSpeedAlt"])}, {fmt(p["recoveryCrouch"])}, {fmt(p["recoveryStand"])}, {{ {modes} }} }},\n'
header+='''};
inline const CS2015WeaponProfile *CSFind2015WeaponProfile(CSWeaponID id) {
    for (int i=0; i<ARRAYSIZE(g_CS2015WeaponProfiles); ++i) if(g_CS2015WeaponProfiles[i].id==id) return &g_CS2015WeaponProfiles[i];
    return NULL;
}
inline void CSApply2015WeaponProfile(CCSWeaponInfo &info, CSWeaponID id) {
    const CS2015WeaponProfile *p=CSFind2015WeaponProfile(id); if(!p) return;
    info.m_flMaxSpeed=p->maxSpeed; info.m_flMaxSpeedAlt=p->maxSpeedAlt;
    info.m_iRecoilSeed=p->seed; info.m_bFullAuto=p->fullAuto;
    info.m_fRecoveryTimeCrouch=info.m_fRecoveryTimeCrouchFinal=p->recoveryCrouch;
    info.m_fRecoveryTimeStand=info.m_fRecoveryTimeStandFinal=p->recoveryStand;
    info.m_iRecoveryTransitionStartBullet=info.m_iRecoveryTransitionEndBullet=0;
    for(int mode=0;mode<2;++mode) {
        const CS2015WeaponMode &m=p->modes[mode];
        info.m_flCycleTime[mode]=m.cycle; info.m_fSpread[mode]=m.spread;
        info.m_fInaccuracyCrouch[mode]=m.crouch; info.m_fInaccuracyStand[mode]=m.stand;
        info.m_fInaccuracyJump[mode]=m.jump; info.m_fInaccuracyJumpInitial[mode]=0;
        info.m_fInaccuracyLand[mode]=m.land; info.m_fInaccuracyLadder[mode]=m.ladder;
        info.m_fInaccuracyImpulseFire[mode]=m.fire; info.m_fInaccuracyMove[mode]=m.move;
        info.m_fRecoilAngle[mode]=m.angle; info.m_fRecoilAngleVariance[mode]=m.angleVariance;
        info.m_fRecoilMagnitude[mode]=m.magnitude; info.m_fRecoilMagnitudeVariance[mode]=m.magnitudeVariance;
    }
}
#endif
'''
dest=SOURCE/'game/shared/cstrike/cs_2015_weapon_profiles.h';dest.write_text(header)
for p in profiles:
    print(p['name'],p['seed'],p['recoveryCrouch'],p['recoveryStand'],p['modes'][0]['magnitude'])
