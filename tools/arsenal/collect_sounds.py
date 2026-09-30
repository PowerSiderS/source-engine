"""Namespace animation sounds per inventory item, without replacing base scripts."""
import argparse,json,re,shutil
from pathlib import Path

def blocks(text):
    text=re.sub(r'//[^\n]*','',text)
    pattern=re.compile(r'"([^"\n]+)"\s*\{');position=0
    while match:=pattern.search(text,position):
        start=match.end();end=start;depth=1;quoted=False
        while end<len(text) and depth:
            char=text[end]
            if char=='"':quoted=not quoted
            if not quoted:depth+=(char=='{')-(char=='}')
            end+=1
        if depth:raise ValueError('Unbalanced sound script')
        yield match.group(1),text[start:end-1]
        position=end

def main():
    p=argparse.ArgumentParser();p.add_argument('--unpacked',type=Path,required=True);p.add_argument('--catalog',type=Path,required=True);p.add_argument('--stage',type=Path,required=True);a=p.parse_args()
    common=a.unpacked/'csgo_mod_---_scripts_and_sounds';base={}
    for path in sorted((common/'scripts/weapons').glob('*.txt')):
        for key,value in blocks(path.read_text(encoding='utf-8-sig')):base[key.lower()]=(key,value)
    rows=json.loads((a.stage/'arsenal_mapping.json').read_text());report=[];manifest=(a.stage/'scripts/skins_manifest.txt').read_text()
    for row in rows:
        slug=Path(row['view_model']).stem.removeprefix('c_weapon_');source=a.unpacked/slug;definitions=base.copy()
        for path in sorted((source/'scripts/weapons').glob('*.txt')):
            for key,value in blocks(path.read_text(encoding='utf-8-sig')):definitions[key.lower()]=(key,value)
        qc=a.catalog.parent/slug/'view.qc';text=re.sub(r'"SA\d+\.([^"\n]+)"',r'"\1"',qc.read_text());id=row['item_id'];events=re.findall(r'event\s+5004\s+\d+\s+"([^"]+)"',text)
        aliases={'Weapon_Deagle.Sideback':'weapons/revolver/revolver_sideback.wav','Weapon_Deagle.Siderelease':'weapons/revolver/revolver_siderelease.wav','Weapon_FiveSeven.Slideback':'weapons/fiveseven/fiveseven_slideback.wav','Weapon_FiveSeven.Sliderelease':'weapons/fiveseven/fiveseven_sliderelease.wav','Weapon_GalilAR.Draw':'weapons/galilar/galil_draw.wav','Weapon_M249.Chain':'weapons/m249/m249_chain.wav','Weapon_M249.Coverdown':'weapons/m249/m249_coverdown.wav'}
        if slug=='m249_m249':aliases['Weapon_M249.Draw']='weapons/m249/m249_draw.wav'
        for key,wave in aliases.items():
            if slug=='desert_eagle_r8_revolver' and wave.startswith('weapons/revolver/'):
                found=list((source/'sound').rglob(Path(wave).name));wave=found[0].relative_to(source/'sound').as_posix() if found else wave
            if key in events and any((root/'sound'/wave).is_file() for root in (source,common)):
                definitions[key.lower()]=(key,'\n "channel" "CHAN_ITEM"\n "volume" "1"\n "soundlevel" "SNDLVL_NORM"\n "pitch" "PITCH_NORM"\n "wave" "'+wave+'"\n')
        host=row.get('script_source_class',row['weapon_class']).removeprefix('weapon_')
        sound_class={'mp5navy':'MP5Navy','m3':'M3','sg550':'SG550','sg552':'SG552','elite':'Elite'}.get(host,host.upper())
        normal='Weapon_'+sound_class+'.Single';silenced='Weapon_'+sound_class+'.Silenced'
        selected=set(events+[normal,silenced]);out=[];missing=[];files=set();renamed={}
        for original in sorted(selected):
            definition=definitions.get(original.lower())
            if definition is None:
                if original in events:missing.append(original)
                continue
            key,body=definition;name=f'SA{id}.{key}';renamed[original]=name
            def replace_wave(match):
                wave=match[1].replace('\\','/');clean=wave.lstrip('*!#><^@)');prefix=wave[:-len(clean)] if clean else ''
                candidates=[source/'sound'/clean,common/'sound'/clean];file=next((x for x in candidates if x.is_file()),None)
                if file is None:missing.append('wave:'+wave);return match[0]
                new=f'sourceadvanced/{id}/{clean}';dest=a.stage/'sound'/new;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(file,dest);files.add(new)
                return '"wave" "'+prefix+new+'"'
            body=re.sub(r'"wave"\s+"([^"]+)"',replace_wave,body,flags=re.I);out.append('"'+name+'"\n{'+body+'}\n')
        for old,new in renamed.items():text=text.replace('"'+old+'"','"'+new+'"')
        qc.write_text(text)
        script=f'scripts/game_sounds_sourceadvanced_{id}.txt';(a.stage/script).write_text(''.join(out))
        additions={'sound_script':script,'shoot_sound':renamed.get(normal,''),'silenced_sound':renamed.get(silenced,'')}
        row.update(additions)
        # Add fields inside this item's own block, preserving deterministic IDs.
        pattern=r'("'+str(id)+r'"\s*\{)';fields=''.join('\n  "'+k+'" "'+v+'"' for k,v in additions.items())
        manifest=re.sub(r'("'+str(id)+r'"\s*\{)([^}]+)',lambda m:m[1]+re.sub(r'\s*"(?:sound_script|shoot_sound|silenced_sound)"\s*"[^"]*"','',m[2]),manifest,count=1)
        manifest=re.sub(pattern,lambda m:m[1]+fields,manifest,count=1)
        report.append({'item':id,'folder':slug,'events':len(events),'sounds':len(out),'files':len(files),'missing':missing})
    (a.stage/'scripts/skins_manifest.txt').write_text(manifest);(a.stage/'arsenal_mapping.json').write_text(json.dumps(rows,indent=2));(a.stage/'sound_audit.json').write_text(json.dumps(report,indent=2))
    print('Sound scripts',len(report),'unresolved',sum(len(x['missing']) for x in report))

if __name__=='__main__':main()
