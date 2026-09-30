import os
from pathlib import Path
import json,shutil,subprocess,sys,hashlib
work=Path(os.environ.get('SA_KNIFE_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'))
root=work/'native-knife-engine-20260930'
source=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
game=Path(r'C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)')
sys.path.insert(0,str(source/'tools'))
from build_gameplay_vpk import verify_vpk
stage=root/'000_sourceadvanced_catalog'
previous=work/'inspect-optimization-20260930/000_sourceadvanced_catalog'
shutil.copytree(previous,stage,dirs_exist_ok=True)
assets=root/'assets/cstrike'
for category in ('models','materials','sound'):
 shutil.copytree(assets/category,stage/category,dirs_exist_ok=True)
shutil.copy2(assets/'scripts/game_sounds_sourceadvanced_native_knives.txt',stage/'scripts/game_sounds_sourceadvanced_native_knives.txt')
# Leave the live inventory manifest loose, so future edits remain effective.
(stage/'scripts/sourceadvanced_native_animation_provenance.json').write_text((root/'asset-validation.json').read_text(),encoding='utf-8')
(stage/'scripts/sourceadvanced_catalog.json').write_text(json.dumps(dict(
 source='Existing local gun catalog plus recompiled existing CSSO knife clips',
 engine_commands=['inspectlook'],model_count=57,
 knife_animation_provenance='Existing native clips, numeric frames unchanged; not authenticated Valve 2015 binaries',
 gun_animation_provenance='Previous local gun catalog, unchanged in this update',
 knife_arms='c_arms_native, 36 original arm bones including forearm twists',
 gun_arms='c_arms_default, previous retargeted 47-bone rig'),indent=2),encoding='utf-8')
vpk=Path(r'C:\Users\SnyX\Downloads\Compressed\csso_release_1.1\bin\vpk.exe')
subprocess.run([str(vpk),str(stage)],cwd=vpk.parent,check=True,creationflags=0x08000000,stdout=(root/'vpk-build.log').open('w'))
package=stage.with_suffix('.vpk')
entries=verify_vpk(package,stage)
report=dict(vpk=str(package),sha256=hashlib.sha256(package.read_bytes()).hexdigest(),files=len(entries),entries=entries)
(root/'package-validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('VPK verified:',len(entries),'files',report['sha256'],flush=True)
